import cv2
import numpy as np
import matplotlib.pyplot as plt
import av

video_path = 'resource/screencap.mp4'

color = np.array([255, 243, 243])
delta = np.array([10, 10, 10])
lb = color - delta
ub = color + delta

ys = []
delays = []
cur_ys = []
cur_pts = []

container = av.open(video_path)
stream = container.streams.video[0]

for frame in container.decode(stream):
    img = frame.to_ndarray(format='bgr24')
    pts_time = float(frame.pts * stream.time_base) * 1000
    mask = cv2.inRange(img[0:720, 1046:1263], lb, ub)
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)

    if len(contours) > 0:
        for c in contours:
            area = cv2.boundingRect(c)
            y = area[1] + area[3] / 2
            cur_ys.append(y)
            cur_pts.append(pts_time)
    else:
        if len(cur_ys) > 2 and cur_ys[0] <= 10 and cur_ys[-1] >= 570:
            end_idx = 0
            for i in range(len(cur_ys)):
                if abs(cur_ys[i] - 564) < abs(cur_ys[end_idx] - 564):
                    end_idx = i
            for i in range(len(cur_ys)):
                ys.append(cur_ys[i])
                delays.append(cur_pts[end_idx] - cur_pts[i])
        elif len(cur_ys) > 2:
            print(len(cur_ys), cur_ys[0], cur_ys[-1])
        cur_ys = []
        cur_pts = []

ys = np.array(ys)
delays = np.array(delays)

degree = 10
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