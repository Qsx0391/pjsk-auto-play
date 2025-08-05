#include "main_window.h"

#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSettings>
#include <QCloseEvent>
#include <QFile>
#include <QDataStream>
#include <QDir>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    SetupUi();
    CreateConnections();
    LoadSettings();
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* event) {
    if (auto_player_) {
        auto_player_->Stop();
    }
    SaveSettings();
    event->accept();
}

void MainWindow::LoadSettings() {
    QSettings settings("PJSK", "Helper");

    // Mumu settings
    mumu_path_edit_->setText(
        settings.value("mumu/path", kDefaultMumuPath).toString());
    mumu_inst_spin_->setValue(
        settings.value("mumu/instance", kDefaultMumuInstance).toInt());
    adb_port_spin_->setValue(
        settings.value("mumu/adb_port", kDefaultAdbPort).toInt());
    package_name_edit_->setText(
        settings.value("mumu/package", kDefaultPackageName).toString());

    const auto& pc = psh::kDefaultPlayConfig;
    // Play settings
    color_delta_spin_->setValue(
        settings.value("play/color_delta", pc.color_delta).toInt());
    hit_delay_spin_->setValue(
        settings.value("play/hit_delay", pc.hit_delay_ms).toInt());
    tap_duration_spin_->setValue(
        settings.value("play/tap_duration", pc.tap_duration_ms).toInt());
    check_loop_delay_spin_->setValue(
        settings.value("play/check_loop_delay", pc.check_loop_delay_ms)
            .toInt());
}

void MainWindow::SaveSettings() {
    QSettings settings("PJSK", "Helper");

    // Mumu settings
    settings.setValue("mumu/path", mumu_path_edit_->text());
    settings.setValue("mumu/instance", mumu_inst_spin_->value());
    settings.setValue("mumu/adb_port", adb_port_spin_->value());
    settings.setValue("mumu/package", package_name_edit_->text());

    // Play settings
    settings.setValue("play/color_delta", color_delta_spin_->value());
    settings.setValue("play/hit_delay", hit_delay_spin_->value());
    settings.setValue("play/tap_duration", tap_duration_spin_->value());
    settings.setValue("play/check_loop_delay", check_loop_delay_spin_->value());
}

void MainWindow::SetupUi() {
    // 创建中央部件
    central_widget_ = new QWidget(this);
    setCentralWidget(central_widget_);

    // 创建主布局
    auto main_layout = new QVBoxLayout(central_widget_);

    // Mumu设置
    auto mumu_group = new QGroupBox("模拟器设置", this);
    auto mumu_layout = new QFormLayout(mumu_group);

    mumu_path_edit_ =
        new QLineEdit("E:/Program Files/Netease/MuMu Player 12", this);
    mumu_path_button_ = new QPushButton("浏览...", this);
    auto mumu_path_layout = new QHBoxLayout();
    mumu_path_layout->addWidget(mumu_path_edit_);
    mumu_path_layout->addWidget(mumu_path_button_);

    mumu_inst_spin_ = new QSpinBox(this);
    mumu_inst_spin_->setRange(0, 9);
    mumu_inst_spin_->setValue(0);

    adb_port_spin_ = new QSpinBox(this);
    adb_port_spin_->setRange(1024, 65535);
    adb_port_spin_->setValue(16384);

    package_name_edit_ = new QLineEdit("default", this);

    mumu_layout->addRow("模拟器路径：", mumu_path_layout);
    mumu_layout->addRow("实例索引：", mumu_inst_spin_);
    mumu_layout->addRow("ADB端口：", adb_port_spin_);
    mumu_layout->addRow("包名：", package_name_edit_);

    main_layout->addWidget(mumu_group);

    // 播放设置
    auto config_group = new QGroupBox("播放设置", this);
    auto config_layout = new QFormLayout(config_group);

    color_delta_spin_ = new QSpinBox(this);
    color_delta_spin_->setRange(1, 255);
    color_delta_spin_->setValue(100);
    config_layout->addRow("最大颜色距离：", color_delta_spin_);

    hit_delay_spin_ = new QSpinBox(this);
    hit_delay_spin_->setRange(0, 10000);
    hit_delay_spin_->setValue(2110);
    config_layout->addRow("点击延迟(ms)：", hit_delay_spin_);

    tap_duration_spin_ = new QSpinBox(this);
    tap_duration_spin_->setRange(1, 100);
    tap_duration_spin_->setValue(20);
    config_layout->addRow("点击持续时间(ms)：", tap_duration_spin_);

    check_loop_delay_spin_ = new QSpinBox(this);
    check_loop_delay_spin_->setRange(1, 100);
    check_loop_delay_spin_->setValue(10);
    config_layout->addRow("检查循环延迟(ms)：", check_loop_delay_spin_);

    main_layout->addWidget(config_group);

    // 控制按钮
    auto button_layout = new QHBoxLayout();
    start_button_ = new QPushButton("开始", this);
    stop_button_ = new QPushButton("停止", this);
    stop_button_->setEnabled(false);
    button_layout->addWidget(start_button_);
    button_layout->addWidget(stop_button_);
    main_layout->addLayout(button_layout);

    // 状态标签
    status_label_ = new QLabel("就绪", this);
    main_layout->addWidget(status_label_);

    // 设置窗口属性
    setWindowTitle("PJSK Helper");
    setMinimumWidth(400);
}

void MainWindow::CreateConnections() {
    connect(start_button_, &QPushButton::clicked, this,
            &MainWindow::OnStartButtonClicked);
    connect(stop_button_, &QPushButton::clicked, this,
            &MainWindow::OnStopButtonClicked);
    connect(mumu_path_button_, &QPushButton::clicked, this,
            &MainWindow::OnMumuPathButtonClicked);
}

void MainWindow::OnMumuPathButtonClicked() {
    QString dir = QFileDialog::getExistingDirectory(
        this, "选择模拟器目录", mumu_path_edit_->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        mumu_path_edit_->setText(dir);
    }
}

psh::PlayConfig MainWindow::GetPlayConfig() const {
    psh::PlayConfig config{};
    config.hold_cnt = 6;
    config.color_delta = color_delta_spin_->value();
    config.tap_duration_ms = tap_duration_spin_->value();
    config.check_loop_delay_ms = check_loop_delay_spin_->value();
    config.hit_delay_ms = hit_delay_spin_->value();
    return config;
}

void MainWindow::InitAutoPlayer() {
    mumu_client_ = std::make_unique<psh::MumuClient>(
        mumu_path_edit_->text(), mumu_inst_spin_->value(),
        package_name_edit_->text());

    note_estimator_ = std::make_unique<psh::NoteTimeEstimator>(
        psh::kDefaultTrackConfig.check_upper_y,
        psh::kDefaultTrackConfig.check_lower_y);

    psh::TrackConfig track_config = psh::kDefaultTrackConfig;

    auto_player_ = std::make_unique<psh::AutoPlayer>(
        *mumu_client_, *mumu_client_, *mumu_client_, *mumu_client_,
        *note_estimator_, track_config, GetPlayConfig());
}

void MainWindow::OnStartButtonClicked() {
    try {
        if (!auto_player_) {
            InitAutoPlayer();
        } else {
            auto_player_->SetPlayConfig(GetPlayConfig());
        }

        if (!auto_player_->Start()) {
            throw std::runtime_error("自动播放器启动失败");
        }

        start_button_->setEnabled(false);
        stop_button_->setEnabled(true);
        status_label_->setText("运行中...");
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "错误",
                              QString("启动失败: %1").arg(e.what()));
    }
}

void MainWindow::OnStopButtonClicked() {
    try {
        if (auto_player_) {
            auto_player_->Stop();
        }

        start_button_->setEnabled(true);
        stop_button_->setEnabled(false);
        status_label_->setText("就绪");
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "错误",
                              QString("停止失败: %1").arg(e.what()));
    }
}