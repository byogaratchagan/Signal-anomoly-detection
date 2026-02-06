import numpy as np
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers
from sklearn.preprocessing import MinMaxScaler

def convert_to_binned_data(timings_list1, timings_list2, bin_size):
    """
    Converts two asynchronous lists of "time-between-toggles"
    into a single, synchronous 2D array of "toggles-per-bin".
    
    Args:
        timings_list1 (list or np.array): Your 'data1' (e.g., clock timings)
        timings_list2 (list or np.array): Your 'data2' (e.g., data timings)
        bin_size (int or float): The "width" of each time bin (e.g., 100)
    
    Returns:
        np.array: A 2D array of shape (num_bins, 2)
    """
    
    # --- Step 1: Reconstruct the absolute timeline for each signal ---
    # np.cumsum() creates the timeline: [167, 167+167, 167+167+225, ...]
    toggle_times1 = np.cumsum(timings_list1)
    toggle_times2 = np.cumsum(timings_list2)
    
    # --- Step 2: Find the total duration ---
    # We need to know when the last toggle happened
    total_duration = max(toggle_times1[-1], toggle_times2[-1])
    
    # --- Step 3: Create the time bins ---
    # np.arange(start, stop, step)
    # This creates bins like [0, 100, 200, 300, ..., total_duration+100]
    bins = np.arange(0, total_duration + bin_size, bin_size)
    
    # --- Step 4: Count toggles in each bin using np.histogram() ---
    # This is the magic. It's super fast.
    counts1, _ = np.histogram(toggle_times1, bins=bins)
    counts2, _ = np.histogram(toggle_times2, bins=bins)
    
    # --- Step 5: Combine the counts into one 2D array ---
    # np.stack() joins them. We use axis=1 to make them columns.
    # The result is: [[counts1_bin1, counts2_bin1],
    #                [counts1_bin2, counts2_bin2],
    #                ...]
    binned_data = np.stack([counts1, counts2], axis=1)
    
    return binned_data

# Your data specifications
WINDOW_SIZE = 10
BIN_SIZE = 100
NUM_FEATURES = 2
INPUT_SHAPE = (WINDOW_SIZE, NUM_FEATURES) # This will be (20, 2)

data1 = [167, 167, 225, 131, 223, 187, 167, 187, 223, 333, 263, 277, 103, 167, 167, 277, 161, 15, 277, 277, 315, 167, 167, 277, 277, 167, 277, 167, 69, 167, 167, 277, 167, 167, 167, 335, 167, 131, 277, 335, 277, 167, 105, 167, 167, 277, 277, 167, 167, 167, 167, 167, 156, 277, 167, 277, 167, 277, 15, 45, 15, 167, 75, 145, 167, 277, 335, 277, 167, 277, 445, 167, 277, 125, 187, 55, 167, 445, 167, 167, 249, 167, 277, 167]
data2 = [37, 37, 53, 25, 57, 45, 37, 45, 57, 61, 59, 45, 6, 39, 39, 63, 37, 4, 43, 43, 59, 39, 41, 45, 43, 39, 45, 39, 17, 39, 39, 43, 37, 37, 39, 61, 37, 11, 45, 74, 43, 39, 27, 37, 37, 45, 43, 39, 37, 37, 37, 37, 17, 3, 16, 43, 39, 45, 37, 43, 5, 11, 5, 39, 19, 13, 37, 43, 77, 43, 41, 43, 83, 37, 43, 31, 39, 15, 37, 85, 39, 37, 55, 37]


def build_simple_autoencoder():
    """
    Builds an autoencoder using Flatten and Dense layers.
    """
    model = keras.Sequential()
    
    # --- Encoder ---
    # Input layer specifies the 2D shape (20, 2)
    model.add(layers.Input(shape=INPUT_SHAPE))
    
    # Flatten layer converts (20, 2) into a 1D vector of 40
    model.add(layers.Flatten())
    
    # Standard Dense layers
    model.add(layers.Dense(32, activation='relu'))
    model.add(layers.Dense(16, activation='relu')) # Bottleneck
    
    # --- Decoder ---
    model.add(layers.Dense(32, activation='relu'))
    
    # Output layer must have 40 units (to match the flattened input)
    model.add(layers.Dense(WINDOW_SIZE * NUM_FEATURES, activation='sigmoid'))
    
    # Reshape layer converts the (40,) vector back to (20, 2)
    model.add(layers.Reshape(INPUT_SHAPE))
    
    # Compile the model
    model.compile(optimizer='adam', loss='mse')
    return model

def create_sliding_windows(data, window_size):
    """
    Converts a 2D array (samples, features) into a 3D array
    (samples, window_size, features).
    """
    all_windows = []
    num_samples = len(data) - window_size + 1
    for i in range(num_samples):
        window = data[i : i + window_size]
        all_windows.append(window)
    return np.array(all_windows)

def train_model(autoencoder, window_size, num_features):
    """
    Trains the autoencoder and returns the model, scaler, and threshold.
    """
    print("Starting model training...")
    
    # 1. PREPARE YOUR DATA
# This is the function from Step 1.
    print("Step 1: Converting to binned data...")
    binned_data = convert_to_binned_data(data1, data2, BIN_SIZE)
    print(f"Binned data shape: {binned_data.shape}")
    print(f"Binned data (first 5 bins):\n {binned_data[:5]}")

    # 2. SCALE THE DATA
    print("\nStep 2: Scaling data...")
    scaler = MinMaxScaler()
    scaled_data = scaler.fit_transform(binned_data)

    # 3. CREATE SLIDING WINDOWS
    print("\nStep 3: Creating sliding windows...")
    X_train = create_sliding_windows(scaled_data, WINDOW_SIZE)
    print(f"Final X_train shape for model: {X_train.shape}")
        
    # 3. Build and train the model
    autoencoder.fit(
        X_train,
        X_train,
        epochs=20,
        batch_size=32,
        shuffle=True,
        validation_split=0.1,
        verbose=1
    )
    
    # 4. Calculate the anomaly threshold
    reconstructions = autoencoder.predict(X_train)
    # Calculate error for each sample
    errors = np.mean(np.square(X_train - reconstructions), axis=(1, 2))
    
    # Use the 99th percentile as the threshold
    threshold = np.quantile(errors, 0.99)
    
    print(f"Training complete. Anomaly threshold set to: {threshold}")
    return autoencoder, scaler, threshold

def detect_anomalies(new_data1, new_data2, model, scaler, bin_size, window_size, threshold):
    """
    Runs the full detection pipeline on new raw data.
    
    Args:
        new_data1 (np.array): New raw timings for signal 1
        new_data2 (np.array): New raw timings for signal 2
        model (keras.Model): Your trained autoencoder
        scaler (MinMaxScaler): Your FITTED scaler
        bin_size (int): The bin size used during training
        window_size (int): The window size used during training
        threshold (float): The calculated anomaly threshold
        
    Returns:
        A list of indices where anomalies were detected.
    """
    print("\n--- Testing new data... ---")
    
    # 1. Binning: Convert raw timings to binned counts
    try:
        binned_test_data = convert_to_binned_data(new_data1, new_data2, bin_size)
        if binned_test_data.shape[0] < window_size:
            print("Error: New data is shorter than the window size.")
            return [], []
    except Exception as e:
        print(f"Error binning new data: {e}")
        return [], []

    # 2. Scaling: Use the *same* scaler (scaler.transform)
    scaled_test_data = scaler.transform(binned_test_data)
    
    # 3. Windowing: Create sliding windows
    test_windows = create_sliding_windows(scaled_test_data, window_size)
    if test_windows.shape[0] == 0:
         print("No windows created from new data.")
         return [], []

    # 4. Prediction: Get reconstructions
    reconstructions = model.predict(test_windows)
    
    # 5. Error: Calculate MSE for each window
    errors = np.mean(np.square(test_windows - reconstructions), axis=(1, 2))
    
    # 6. Decision: Find windows where error > threshold
    anomaly_indices = np.where(errors > threshold)[0]
    
    return errors, anomaly_indices

# --- Build and check the model ---
simple_autoencoder = build_simple_autoencoder()
simple_autoencoder.summary()
    
model, scaler, anomaly_threshold = train_model(simple_autoencoder, WINDOW_SIZE, NUM_FEATURES)

# (This goes at the end of your script)

# --- Test 1: Test with NORMAL data ---
# (We'll just re-use our training data as a "new" file)
print("\n--- TEST 1: (Using Normal Data) ---")
errors, indices = detect_anomalies(
    data1, 
    data2, 
    simple_autoencoder, 
    scaler, 
    BIN_SIZE, 
    WINDOW_SIZE, 
    anomaly_threshold
)

if len(indices) > 0:
    print(f"WARNING: Anomalies detected in normal data at indices: {indices}")
else:
    print("SUCCESS: No anomalies detected in normal data.")


# --- Test 2: Test with ANOMALOUS data ---
print("\n--- TEST 2: (Using Simulated Anomaly) ---")

# Let's create an anomaly:
# We'll make one of the clock timings HUGE (like your 9323)
new_data1 = data1.copy()
new_data1[10] = 50000  # This will create a bin with 0 counts for a long time

# (You could also just change a bin count)
# For example, if you binned your data, you could do:
# binned_data[50, 0] = 1000 # (1000 clock toggles in one bin)

errors, indices = detect_anomalies(
    new_data1, 
    data2, 
    simple_autoencoder, 
    scaler, 
    BIN_SIZE, 
    WINDOW_SIZE, 
    anomaly_threshold
)

if len(indices) > 0:
    print(f"SUCCESS: Anomaly detected at window indices: {indices}")
    print(f"Error at first anomaly (window {indices[0]}): {errors[indices[0]]}")
    print(f"Threshold was: {anomaly_threshold}")
else:
    print("FAILURE: Model did not detect the anomaly.")


