import numpy as np
import tensorflow as tf
import keras
from keras import layers
from sklearn.preprocessing import MinMaxScaler

from numpy.lib.stride_tricks import as_strided

def create_sliding_windows(flat_array, window_shape):
    """
    Creates overlapping (sliding) windows from a flat array.
    
    Args:
        flat_array (np.array): Your 1D array.
        window_shape (tuple): The 2D shape you want, e.g., (8, 8).
    """
    
    # 1. Calculate the 1D size of your window
    window_size_flat = window_shape[0] * window_shape[1] # 8 * 8 = 64
    
    # 2. Calculate how many windows you can make
    # (N - L + 1)
    num_windows = len(flat_array) - window_size_flat + 1
    
    # 3. Define the "strides" (how to move in memory)
    # arr.strides[0] is the size (in bytes) of one element
    byte_stride = flat_array.strides[0]
    
    # To get to the next window, step by 1 element (byte_stride)
    # To get to the next element *within* a window, step by 1 element (byte_stride)
    strides = (byte_stride, byte_stride)
    
    # 4. Create the 2D view of all windows
    # This will have shape (num_windows, 64)
    windows_2d = as_strided(
        flat_array,
        shape=(num_windows, window_size_flat),
        strides=strides
    )
    
    # 5. Reshape this 2D view into a 3D view
    # (num_windows, 8, 8)
    return windows_2d.reshape(num_windows, *window_shape)

FEATURES = 8
WINDOW_SIZE=8

data1 = [167, 167, 225, 131, 223, 187, 167, 187, 223, 333, 263, 277, 103, 167, 167, 277, 161, 15, 277, 277, 315, 167, 167, 277, 277, 167, 277, 167, 69, 167, 167, 277, 167, 167, 167, 335, 167, 131, 277, 335, 277, 167, 105, 167, 167, 277, 277, 167, 167, 167, 167, 167, 156, 277, 167, 277, 167, 277, 15, 45, 15, 167, 75, 145, 167, 277, 335, 277, 167, 277, 445, 167, 277, 125, 187, 55, 167, 445, 167, 167, 249, 167, 277, 167]
data2 = [37, 37, 53, 25, 57, 45, 37, 45, 57, 61, 59, 45, 6, 39, 39, 63, 37, 4, 43, 43, 59, 39, 41, 45, 43, 39, 45, 39, 17, 39, 39, 43, 37, 37, 39, 61, 37, 11, 45, 74, 43, 39, 27, 37, 37, 45, 43, 39, 37, 37, 37, 37, 17, 3, 16, 43, 39, 45, 37, 43, 5, 11, 5, 39, 19, 13, 37, 43, 77, 43, 41, 43, 83, 37, 43, 31, 39, 15, 37, 85, 39, 37, 55, 37]

def build_model():
    model = keras.Sequential()
    model.add(layers.Input(shape=(FEATURES, WINDOW_SIZE)))
    model.add(layers.Flatten())
    model.add(layers.Dense(32, activation='relu'))
    model.add(layers.Dense(16, activation='relu'))
    model.add(layers.Dense(32, activation='relu'))
    model.add(layers.Dense(WINDOW_SIZE * FEATURES, activation='sigmoid'))
    model.add(layers.Reshape((FEATURES, WINDOW_SIZE)))
    model.compile(optimizer='adam', loss='mse')
    return model

model = build_model()

if (len(data1) > len(data2)):
    data1 = data1[:-len(data2)]
elif (len(data1) < len(data2)):
    data2 = data2[:-len(data1)]

stack = np.column_stack((data1, data2)).flatten()
scaled_data = (stack - stack.min()) / (stack.max()-stack.min())
X_train = create_sliding_windows(stack, (FEATURES, WINDOW_SIZE))

model.fit(
    X_train,
    X_train,
    epochs=20,
    batch_size=32,
    shuffle=True,
    validation_split=0.1,
    verbose=1)

reconstructions = model.predict(X_train)
    # Calculate error for each sample
errors = np.mean(np.square(X_train - reconstructions), axis=(1, 2))

# Use the 99th percentile as the threshold
threshold = np.quantile(errors, 0.99)

print(f"Training complete. Anomaly threshold set to: {threshold}")

