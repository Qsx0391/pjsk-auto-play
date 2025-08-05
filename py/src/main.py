import cv2
import numpy as np
import matplotlib.pyplot as plt

color = np.array([255, 243, 243])
delta = np.array([10, 10, 10])
lb = color - delta
ub = color + delta
begin = 8
end = 86
ys = []
delays = []
for i in range(begin, end + 1):
    img = cv2.imread(f'../source/frames/frame_{i:04d}.png')
    mask = cv2.inRange(img, lb, ub)
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
    delay_time_ms = (end - i) * 1000 / 60
    for c in contours:
        area = cv2.boundingRect(c)
        y = area[1] + area[3]/2
        print(f'{i:04d}: {y}, {delay_time_ms}')
        ys.append(y)
        delays.append(delay_time_ms)

ys = np.array(ys)
delays = np.array(delays)

degree = 4
coeffs = np.polyfit(ys, delays, deg=degree)
fitted_fn = np.poly1d(coeffs)

print("拟合表达式:")
print(fitted_fn)

plt.scatter(ys, delays, label='data points')
y_range = np.linspace(min(ys), max(ys), 500)
plt.plot(y_range, fitted_fn(y_range), color='red', label='fitted curve')
plt.xlabel("y")
plt.ylabel("delay_time_ms")
plt.legend()
plt.grid()
plt.show()