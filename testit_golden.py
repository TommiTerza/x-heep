# This is a template for the golden function(s) that can be used to define the expected behaviour 
# of TestIt tests. The golden function(s) should use the NumPy library, as TestIt produces NumPy tensors
# as input data and expects NumPy tensors as output data. 

import numpy as np
import torch
import torch.nn.functional as F

# inputs and parameters arguments are MANDATORY
# output as a list is MANDATORY
def im2col_function(inputs, parameters):
    inputs = np.array(inputs)

    # Extract parameters
    batch_size = next(item['value'] for item in parameters if item['name'] == 'BATCH') 
    channels = next(item['value'] for item in parameters if item['name'] == 'CH')
    image_height = next(item['value'] for item in parameters if item['name'] == 'IH')
    image_width = next(item['value'] for item in parameters if item['name'] == 'IW')
    top_pad = next(item['value'] for item in parameters if item['name'] == 'TOP_PAD')
    bottom_pad = next(item['value'] for item in parameters if item['name'] == 'BOTTOM_PAD')
    left_pad = next(item['value'] for item in parameters if item['name'] == 'LEFT_PAD')
    right_pad = next(item['value'] for item in parameters if item['name'] == 'RIGHT_PAD')
    stride_d1 = next(item['value'] for item in parameters if item['name'] == 'STRIDE_D1')
    stride_d2 = next(item['value'] for item in parameters if item['name'] == 'STRIDE_D2')
    (filter_height, filter_width) = (next(item['value'] for item in parameters if item['name'] == 'FH'), next(item['value'] for item in parameters if item['name'] == 'FW'))
    kernel_size = (filter_height, filter_width)

    # Convert the input array into a PyTorch tensor with the correct shape
    input_tensor = torch.tensor(inputs).view(batch_size, channels, image_height, image_width)

    dilation = 1
    # Ensure kernel_size, stride, padding, and dilation are tuples
    if isinstance(kernel_size, int):
        kernel_size = (kernel_size, kernel_size)
    if isinstance(dilation, int):
        dilation = (dilation, dilation)

    # Adjust padding format for F.pad (expects pad_left, pad_right, pad_top, pad_bottom)
    padding_format = (left_pad, right_pad, top_pad, bottom_pad)

    # Apply zero padding
    padded_input = F.pad(input_tensor, padding_format, "constant", 0)

    # Unfold the padded input tensor
    unfolded = padded_input.unfold(2, kernel_size[0], stride_d2).unfold(3, kernel_size[1], stride_d1)
    unfolded = unfolded.permute(0, 2, 3, 1, 4, 5)

    # Reshape to get the 2D tensor where each row is a flattened receptive field
    channel_dim = padded_input.size(1)
    unfolded_tensor = unfolded.contiguous().view(-1, channel_dim * kernel_size[0] * kernel_size[1]).t()

    # Convert the PyTorch tensor to a NumPy array and then to a list (simple array)
    unfolded_array = unfolded_tensor.numpy()

    return unfolded_array.reshape(1, 1, 1, *unfolded_array.shape)
