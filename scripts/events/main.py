import os
import cv2
import shutil

SRC_DIR = "./resource/src"
DST_DIR = "./resource/events"
QRC_DIR = "./resource/events.qrc"

os.makedirs(DST_DIR, exist_ok=True)

qrc_files = []

# 处理根目录文件
for fname in os.listdir(SRC_DIR):
    src_path = os.path.join(SRC_DIR, fname)
    dst_path = os.path.join(DST_DIR, fname)

    if os.path.isfile(src_path):
        # 根目录文件直接复制
        shutil.copy2(src_path, dst_path)
        rel_path = os.path.relpath(dst_path, SRC_DIR).replace("\\", "/")
        qrc_files.append(rel_path)

# 处理子目录
for subdir in os.listdir(SRC_DIR):
    subdir_path = os.path.join(SRC_DIR, subdir)
    if not os.path.isdir(subdir_path) or subdir == "events":
        continue

    dst_subdir_path = os.path.join(DST_DIR, subdir)
    os.makedirs(dst_subdir_path, exist_ok=True)

    for fname in os.listdir(subdir_path):
        src_file = os.path.join(subdir_path, fname)
        dst_file = os.path.join(dst_subdir_path, fname)

        def append_qrc():
            rel_path = os.path.relpath(dst_file, SRC_DIR).replace("\\", "/")
            qrc_files.append(rel_path)

        if not fname.lower().endswith(".png"):
            shutil.copy2(src_file, dst_file)
            append_qrc()
            continue

        # 判断是否是 x_y.png 或 x_y_w_h.png
        parts = fname[:-4].split("_")
        if len(parts) != 2 and len(parts) != 4:
            shutil.copy2(src_file, dst_file)
            append_qrc()
            continue

        if len(parts) == 2:
            x, y = map(int, parts)
            w = h = 50
        else:
            x, y, w, h = map(int, parts)

        img = cv2.imread(src_file)
        if img is None:
            print(f"无法读取图像: {src_file}")
            continue

        sub_img = img[y:y + h, x:x + w]
        if sub_img.size == 0:
            print(f"截取范围超出边界: {fname}")
            continue

        dst_file = os.path.join(dst_subdir_path, f"{x}_{y}.png")
        cv2.imwrite(dst_file, sub_img)
        append_qrc()
        print(f"已保存裁剪: {dst_file}")

with open(QRC_DIR, "w", encoding="utf-8") as f:
    f.write("<RCC>\n    <qresource prefix=\"/\">\n")
    for path in sorted(qrc_files):
        f.write(f"        <file>{path[1:]}</file>\n")
    f.write("    </qresource>\n</RCC>\n")