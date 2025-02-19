# This is a template for the golden function(s) that can be used to define the expected behaviour 
# of VerifIt tests. The golden function(s) should use the NumPy library, as VerifIt produces NumPy tensors
# as input data and expects NumPy tensors as output data. 

import numpy as np

# inputs and parameters arguments are MANDATORY
# output as a list is MANDATORY
def matrix_multiply(inputs, parameters):
    """
    Multiplies two NumPy matrices and returns the result.
    """
    A = inputs[0]
    B = inputs[1]

    if A.shape[1] != B.shape[0]:  # Check if multiplication is valid
        raise ValueError("Matrix dimensions do not match for multiplication!")

    return [np.matmul(A, B)]  # Perform matrix multiplication