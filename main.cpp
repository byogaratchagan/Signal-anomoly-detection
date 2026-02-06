#include <pico/stdio.h>
#include <pico/stdlib.h>
#include <pico/stdio.h>
#include <stdio.h>
#include <hardware/pio.h>
#include <shift_reg.pio.h>
#include <hardware/dma.h>
#include <hardware/timer.h>
#include <hardware/clocks.h>
#include <hardware/adc.h>

#include "tensorflow/lite/micro/micro_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
// #include "tensorflow/lite/micro/"
#include "model_data.h"

// #using namespace std;

#define CAPTURE_CHANNEL 0
#define CAPTURE_DEPTH 10000

#define ANOMALY_THRESHOLD 0.030633f
const float scaler_min[] = { -0.8f, 0.0f };
const float scaler_scale[] = { 0.06666666666666667f, 0.09090909090909091f };

const int kWindowSize = 10;
const int kNumFeatures = 2;
const int kNumInputs = kWindowSize * kNumFeatures; // 10*2 = 20

// ==============================================================================
// --- 3. TFLITE MICRO GLOBALS ---
// ==============================================================================
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

// A block of memory for TFLite to use.
// You MUST tune this size. If it's too small, AllocateTensors() will fail.
constexpr int kTensorArenaSize = 30 * 1024; // 30KB
uint8_t tensor_arena[kTensorArenaSize];

/*
Pre training:
1.Recording - (combinational work of shift registers, mux, ADC, internal memory).
2.AI analysis on classification of work of each pin(clk, data, chip select, etc).
3.AI analysis on timing characteristics.
4.AI analysis on analog data characteristics.
5.AI analysis on whether it is a 8, 16, 32 or 64 bit communication.
*/

/*
seperate the signals based on long intervals, so find the long interval for each pin in training and split it accordingly.
compare these datasets to the previously trained datasets. using neural network model.
and sigmoid function to detect the anamoly.
*/
uint8_t data = 0;
dma_channel_config_t c;
// uint8_t data_lis[8][CAPTURE_DEPTH];  
uint8_t data_list[CAPTURE_DEPTH];
uint offset;
int dma;
int pio;
int j = 0;
bool finished = false;

int8_t quantize_float_to_int8(float float_val, float scale, int8_t zero_point) {
    // Formula: int8 = (float / scale) + zero_point
    int32_t int_val = (int32_t)(round(float_val / scale) + zero_point);
    // Clamp to int8 range
    if (int_val < -128) int_val = -128;
    if (int_val > 127) int_val = -127;
    return (int8_t)int_val;
}

// Converts the model's int8_t output back to a 0.0-1.0 float
float dequantize_int8_to_float(int8_t int_val, float scale, int8_t zero_point) {
    // Formula: float = (int8 - zero_point) * scale
    return ((float)int_val - (float)zero_point) * scale;
}

void digitalread_irq(){
    dma_hw->ints0 = 1u << dma;
    dma_channel_set_write_addr(dma, data_list, true);
}

void start_read(){
    j = 0;
    finished = false;
    channel_config_set_write_increment(&c, true);
    channel_config_set_read_increment(&c, false);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, DREQ_PIO1_RX0+pio);

    dma_channel_configure(dma, 
        &c,
        data_list,
        &pio1_hw->rxf[pio],
        CAPTURE_DEPTH,
        true
    );
    pio_sm_set_enabled(pio1, pio, true);
    dma_channel_wait_for_finish_blocking(dma);

    pio_sm_set_enabled(pio1, pio, true);
    dma_channel_set_irq0_enabled(dma, true);
    irq_set_exclusive_handler(DMA_IRQ_0, digitalread_irq);
    irq_set_enabled(DMA_IRQ_0, true);

    digitalread_irq();
}

void initiate_read_sequence(){
    pio = pio_claim_unused_sm(pio1, true);
    offset = pio_add_program(pio1, &shift_reg_program);
    shift_reg_init(pio1, pio, offset, 0, 1, 2, (float)clock_get_hz(clk_sys) / (2000000 * 8));
    

    dma = dma_claim_unused_channel(false);

    c = dma_channel_get_default_config(dma);
}

void initiate_adc_read(){
    adc_gpio_init(26);
    adc_init();
    adc_select_input(26);
    adc_fifo_setup(
        true,    // Write each completed conversion to the sample FIFO
        true,    // Enable DMA data request (DREQ)
        1,       // DREQ (and IRQ) asserted when at least 1 sample present
        false,   // We won't see the ERR bit because of 8 bit reads; disable.
        false     // Shift each sample to 8 bits when pushing to FIFO
    );

    // Divisor of 0 -> full speed. Free-running capture with the divider is
    // equivalent to pressing the ADC_CS_START_ONCE button once per `div + 1`
    // cycles (div not necessarily an integer). Each conversion takes 96
    // cycles, so in general you want a divider of 0 (hold down the button
    // continuously) or > 95 (take samples less frequently than 96 cycle
    // intervals). This is all timed by the 48 MHz ADC clock.
    adc_set_clkdiv(0);

    printf("Arming DMA\n");
    sleep_ms(1000);
    // Set up the DMA to start transferring data as soon as it appears in FIFO
    uint dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);

    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, false);

    // Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&cfg, DREQ_ADC);

    dma_channel_configure(dma_chan, &cfg,
        &data,    // dst
        &adc_hw->fifo,  // src
        1,  // transfer count
        true            // start immediately
    );

    printf("Starting capture\n");
    adc_run(true);

    // Once DMA finishes, stop any new conversions from starting, and clean up
    // the FIFO in case the ADC was still mid-conversion.
    dma_channel_wait_for_finish_blocking(dma_chan);
    printf("Capture finished\n");
    adc_run(false);
    adc_fifo_drain();
    printf("Analog Data: %d", data);
}

void print_data(){
    uint8_t data_lis[8][CAPTURE_DEPTH];
    for (int x = 0; x < 1000; x++){
        // for (int i = 0; i < 8; i++){
        start_read();
        for (int j = 0; j < CAPTURE_DEPTH; j++){
            for (int i = 0; i < 8; i++){
                data_lis[i][j] = (data_list[j] >> i) & 1;
            }
        }
        
        for (int i = 0; i < 8; i++){
            printf("%d\n", i);
            for (int j =0; j < CAPTURE_DEPTH; j++){
                printf("%d", data_lis[i][j]);
            }
            printf("\n");
        }
    }
}

void predict(){
    printf("--- Pico TFLite Anomaly Detection ---\n");
    // Load the model from our .h file
    model = tflite::GetModel(anomaly_model_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        error_reporter->Report("Model schema version mismatch!");
        return;
    }

    // Pull in all TFLite operations
    static tflite::MicroMutableOpResolver<5> resolver;
    resolver.AddFullyConnected();
    resolver.AddSoftmax();
    resolver.AddReshape();

    // Create the interpreter
    tflite::MicroInterpreter interpreter(model, resolver, tensor_arena, kTensorArenaSize);
    if (interpreter.AllocateTensors() != kTfLiteOk) {
        printf("Tensor allocation failed!\n");
        while (1);
    }


    // Get pointers to the input and output tensors
    input = interpreter.input(0);
    output = interpreter.output(0);

    // Our model is INT8 quantized
    printf("Model Input type: %d (9=INT8)\n", input->type);
    printf("Model Output type: %d (9=INT8)\n", output->type);

    while(true){
        float test_window[kWindowSize][kNumFeatures] = {
        {2.0f, 1.0f}, {1.0f, 0.0f}, {2.0f, 0.0f}, {1.0f, 1.0f}, {2.0f, 0.0f},
        {1.0f, 0.0f}, {2.0f, 0.0f}, {1.0f, 1.0f}, {2.0f, 0.0f}, {1.0f, 0.0f}
        };

        // --- 2. PRE-PROCESS AND FILL INPUT TENSOR ---
        // We must manually scale (0-1) and quantize (to int8)
        
        // Get the model's input quantization parameters
        float input_scale = input->params.scale;
        int8_t input_zero_point = input->params.zero_point;

        // A buffer to hold our scaled (0-1) floats
        float scaled_input_buffer[kNumInputs]; 
        int input_index = 0;
        
        for (int i = 0; i < kWindowSize; i++) {
            for (int j = 0; j < kNumFeatures; j++) {
                // A. Get the raw bin count
                float raw_val = test_window[i][j];
                
                // B. Scale it (0-1) using params from Python
                // formula: (value - min) * scale
                float scaled_val = (raw_val - scaler_min[j]) * scaler_scale[j];
                
                // C. Clamp (0-1) just in case
                if (scaled_val < 0.0f) scaled_val = 0.0f;
                if (scaled_val > 1.0f) scaled_val = 1.0f;
                
                // D. Store the scaled float. We need it to calculate MSE later.
                scaled_input_buffer[input_index] = scaled_val;
                
                // E. Quantize the float to int8 and put it in the TFLite input tensor
                input->data.int8[input_index] = quantize_float_to_int8(scaled_val, input_scale, input_zero_point);
                
                input_index++;
            }
        }

        // --- 3. RUN THE MODEL ---
        if (interpreter.Invoke() != kTfLiteOk) {
            error_reporter->Report("Invoke failed");
            return;
        }

        // --- 4. POST-PROCESS AND CHECK FOR ANOMALY ---
        
        // Get the model's output quantization parameters
        float output_scale = output->params.scale;
        int8_t output_zero_point = output->params.zero_point;
        float mse = 0.0f; // Mean Squared Error

        for (int i = 0; i < kNumInputs; i++) {
            // A. Get the model's int8 output
            int8_t output_val_int8 = output->data.int8[i];
            
            // B. De-quantize it back to a 0-1 float
            float reconstructed_val = dequantize_int8_to_float(output_val_int8, output_scale, output_zero_point);
            
            // C. Get our original scaled input value
            float original_val = scaled_input_buffer[i];
            
            // D. Calculate squared error
            float error = original_val - reconstructed_val;
            mse += error * error;
        }
        
        // Get the final Mean Squared Error
        mse = mse / (float)kNumInputs;

        // --- 5. MAKE THE DECISION ---
        printf("Calculated MSE: %f\n", mse);
        if (mse > ANOMALY_THRESHOLD) {
            printf("!!! ANOMALY DETECTED !!!\n");
            // TODO: Blink an LED, send a serial message, etc.
        } else {
            printf("System normal.\n");
        }

        // Wait 2 seconds before running again
        sleep_ms(2000);
    }
}

int main(){
    stdio_init_all();
    sleep_ms(5000);
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, true);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    initiate_read_sequence();
    
    // print_data();
    predict();
    
    return 0;
}
