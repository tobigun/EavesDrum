# plots the velocity curves for the curve types

import numpy as np
import matplotlib.pyplot as plt

def curveFunc(inputValue, base, max_input=1023, max_output=127):
    curveValue = np.power(base, inputValue - 1) - 1
    curveMax = np.power(base, max_input - 1) - 1
    outputValue = curveValue / curveMax * (max_output - 1)
    return np.round(outputValue) + 1

input_values = np.arange(1, 1024)
output_values1 = [curveFunc(x, 0.996) for x in input_values]
output_values2 = [curveFunc(x, 0.998) for x in input_values]
output_values3 = [curveFunc(x, 1.002) for x in input_values]
output_values4 = [curveFunc(x, 1.004) for x in input_values]
output_values5 = [x >> 3 for x in input_values]

plt.plot(input_values, output_values1)
plt.plot(input_values, output_values2)
plt.plot(input_values, output_values3)
plt.plot(input_values, output_values4)
plt.plot(input_values, output_values5)
plt.xlabel("Sensor Input Value")
plt.ylabel("MIDI Velocity Output")
plt.title("MIDI Velocity Curve")
plt.grid(True)
plt.show()
