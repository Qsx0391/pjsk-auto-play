#include "main_window.h"
#include "qt_log_sink.h"

#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSettings>
#include <QCloseEvent>
#include <QFile>
#include <QDir>
#include <QTextEdit>
#include <QSplitter>
#include <QGroupBox>
#include <QScrollBar>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace psh {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    SetupUi();
    CreateConnections();
    LoadSettings();

    // 初始化日志系统
    auto qt_sink = std::make_shared<QtLogSink>(this);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    auto logger = std::make_shared<spdlog::logger>(
        "main", spdlog::sinks_init_list{qt_sink, console_sink});
    logger->set_level(spdlog::level::info);
    spdlog::set_default_logger(logger);

    spdlog::info("PJSK Auto Play 已启动");
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* event) {
    if (auto_player_) {
        if (auto_check_enabled_checkbox_->isChecked()) {
            auto_player_->AutoCheckStop();
        } else {
            auto_player_->Stop();
        }
    }
    if (multi_controller_) {
        multi_controller_->Stop();
    }
    SaveSettings();
    event->accept();
}

void MainWindow::LoadSettings() {
    QSettings settings("PJSKAutoPlay");

    // Mumu settings
    mumu_path_edit_->setText(
        settings.value("mumu/path", kDefaultMumuPath).toString());
    mumu_inst_spin_->setValue(
        settings.value("mumu/instance", kDefaultMumuInstance).toInt());
    // adb_port_spin_->setValue(
    //     settings.value("mumu/adb_port", kDefaultAdbPort).toInt());
    // package_name_edit_->setText(
    //     settings.value("mumu/package", kDefaultPackageName).toString());

    // Play settings
    const auto& pc = kDefaultPlayConfig;
    color_delta_spin_->setValue(
        settings.value("play/color_delta", pc.color_delta).toInt());
    hit_delay_spin_->setValue(
        settings.value("play/hit_delay", pc.hit_delay_ms).toInt());
    tap_duration_spin_->setValue(
        settings.value("play/tap_duration", pc.tap_duration_ms).toInt());
    sample_freq_spin_->setValue(
        settings.value("play/sample_freq", 1000 / pc.check_loop_delay_ms)
            .toInt());

    // 画面显示设置
    img_show_checkbox_->setChecked(
        settings.value("play/img_show", false).toBool());

    // Auto check settings
    auto_check_enabled_checkbox_->setChecked(
        settings.value("auto_check/enabled", false).toBool());

    // 协力设置
    multi_mode_combo_->setCurrentIndex(settings.value("multi/mode", 0).toInt());
    multi_max_diff_combo_->setCurrentIndex(
        settings.value("multi/max_diff", 4).toInt());
}

void MainWindow::SaveSettings() {
    QSettings settings("PJSKAutoPlay");

    // Mumu settings
    settings.setValue("mumu/path", mumu_path_edit_->text());
    settings.setValue("mumu/instance", mumu_inst_spin_->value());
    // settings.setValue("mumu/adb_port", adb_port_spin_->value());
    // settings.setValue("mumu/package", package_name_edit_->text());

    // Play settings
    settings.setValue("play/color_delta", color_delta_spin_->value());
    settings.setValue("play/hit_delay", hit_delay_spin_->value());
    settings.setValue("play/tap_duration", tap_duration_spin_->value());
    settings.setValue("play/sample_freq", sample_freq_spin_->value());
    settings.setValue("play/img_show", img_show_checkbox_->isChecked());

    // Auto check settings
    settings.setValue("auto_check/enabled",
                      auto_check_enabled_checkbox_->isChecked());

    // 协力设置
    settings.setValue("multi/mode", multi_mode_combo_->currentIndex());
    settings.setValue("multi/max_diff", multi_max_diff_combo_->currentIndex());
}

void MainWindow::SetupUi() {
    // 创建中央部件
    central_widget_ = new QWidget(this);
    setCentralWidget(central_widget_);

    // 创建主分割器
    main_splitter_ = new QSplitter(Qt::Horizontal, central_widget_);
    auto main_layout = new QHBoxLayout(central_widget_);
    main_layout->addWidget(main_splitter_);

    // 创建左侧控制面板
    auto left_widget = new QWidget();
    auto left_layout = new QVBoxLayout(left_widget);

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

    /*
    adb_port_spin_ = new QSpinBox(this);
    adb_port_spin_->setRange(1024, 65535);
    adb_port_spin_->setValue(16384);
    */

    // package_name_edit_ = new QLineEdit("default", this);

    mumu_layout->addRow("mumu安装路径：", mumu_path_layout);
    mumu_layout->addRow("实例编号：", mumu_inst_spin_);
    // mumu_layout->addRow("ADB端口：", adb_port_spin_);
    // mumu_layout->addRow("包名：", package_name_edit_);

    left_layout->addWidget(mumu_group);

    // 播放设置
    auto config_group = new QGroupBox("播放设置", this);
    auto config_layout = new QFormLayout(config_group);

    color_delta_spin_ = new QSpinBox(this);
    color_delta_spin_->setRange(1, 255);
    color_delta_spin_->setValue(10);
    config_layout->addRow("颜色容差：", color_delta_spin_);

    hit_delay_spin_ = new QSpinBox(this);
    hit_delay_spin_->setRange(-1000, 1000);
    hit_delay_spin_->setValue(0);
    config_layout->addRow("点击延迟(ms)：", hit_delay_spin_);

    tap_duration_spin_ = new QSpinBox(this);
    tap_duration_spin_->setRange(1, 100);
    tap_duration_spin_->setValue(20);
    config_layout->addRow("点击持续时间(ms)：", tap_duration_spin_);

    sample_freq_spin_ = new QSpinBox(this);
    sample_freq_spin_->setRange(1, 1000);
    sample_freq_spin_->setValue(50);
    config_layout->addRow("检查频率(hz)：", sample_freq_spin_);

    left_layout->addWidget(config_group);

    // Auto Check 设置
    auto auto_check_group = new QGroupBox("自动打歌控制", this);
    auto auto_check_layout = new QVBoxLayout(auto_check_group);

    auto_check_enabled_checkbox_ = new QCheckBox("启用自动检测", this);
    auto_check_layout->addWidget(auto_check_enabled_checkbox_);

    // 画面显示开关
    img_show_checkbox_ = new QCheckBox("显示画面", this);
    img_show_checkbox_->setChecked(false);
    auto_check_layout->addWidget(img_show_checkbox_);

    auto start_stop_layout = new QHBoxLayout();
    start_button_ = new QPushButton("开始", this);
    stop_button_ = new QPushButton("停止", this);
    stop_button_->setEnabled(false);

    start_stop_layout->addWidget(start_button_);
    start_stop_layout->addWidget(stop_button_);

    auto_check_layout->addLayout(start_stop_layout);

    left_layout->addWidget(auto_check_group);

    // 协力自动控制设置
    auto multi_group = new QGroupBox("协力自动控制", this);
    auto multi_layout = new QFormLayout(multi_group);

    multi_mode_combo_ = new QComboBox(this);
    multi_mode_combo_->addItems(
        {"固定难度", "优先未通关", "优先非FC", "优先非AP"});

    multi_max_diff_combo_ = new QComboBox(this);
    multi_max_diff_combo_->addItems(
        {"Easy", "Normal", "Hard", "Expert", "Master", "Append"});
    multi_max_diff_combo_->setCurrentIndex(2); // 默认 Hard

    auto multi_btn_layout = new QHBoxLayout();
    multi_start_button_ = new QPushButton("开始协力", this);
    multi_stop_button_ = new QPushButton("停止协力", this);
    multi_stop_button_->setEnabled(false);

    multi_btn_layout->addWidget(multi_start_button_);
    multi_btn_layout->addWidget(multi_stop_button_);

    multi_layout->addRow("模式：", multi_mode_combo_);
    multi_layout->addRow("最大难度：", multi_max_diff_combo_);
    multi_layout->addRow(multi_btn_layout);

    left_layout->addWidget(multi_group);

    // 状态标签
    status_label_ = new QLabel("状态: 未开始", this);
    left_layout->addWidget(status_label_);

    // 添加弹簧让控件顶端对齐
    left_layout->addStretch();

    // 右侧日志显示区域
    auto right_widget = new QWidget();
    auto right_layout = new QVBoxLayout(right_widget);

    // 日志控制区域
    auto log_control_layout = new QHBoxLayout();
    log_level_combo_ = new QComboBox(this);
    log_level_combo_->addItems({"Debug", "Info", "Warn", "Error"});
    log_level_combo_->setCurrentText("Info");

    clear_log_button_ = new QPushButton("清除日志", this);

    log_control_layout->addWidget(new QLabel("日志级别："));
    log_control_layout->addWidget(log_level_combo_);
    log_control_layout->addStretch();
    log_control_layout->addWidget(clear_log_button_);

    right_layout->addLayout(log_control_layout);

    // 日志显示区域
    log_display_ = new QTextEdit(this);
    log_display_->setReadOnly(true);
    log_display_->setFont(QFont("Consolas", 9));
    right_layout->addWidget(log_display_);

    // 添加到分割器
    main_splitter_->addWidget(left_widget);
    main_splitter_->addWidget(right_widget);
    main_splitter_->setStretchFactor(0, 0);
    main_splitter_->setStretchFactor(1, 1);
    main_splitter_->setSizes({400, 600});

    // 设置窗口属性
    setWindowTitle("PJSK Auto Play");
    setMinimumSize(800, 500);
}

void MainWindow::CreateConnections() {
    connect(start_button_, &QPushButton::clicked, this,
            &MainWindow::OnStartButtonClicked);
    connect(stop_button_, &QPushButton::clicked, this,
            &MainWindow::OnStopButtonClicked);
    connect(mumu_path_button_, &QPushButton::clicked, this,
            &MainWindow::OnMumuPathButtonClicked);
    connect(auto_check_enabled_checkbox_, &QCheckBox::toggled, this,
            [this](bool checked) {
                // 状态变化时的处理逻辑（如果需要的话）
            });

    // 图片显示开关信号连接
    connect(img_show_checkbox_, &QCheckBox::toggled, this,
            [this](bool checked) {
                if (auto_player_) {
                    auto_player_->SetImgShowFlag(checked);
                }
            });

    connect(log_level_combo_,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::OnLogLevelChanged);
    connect(clear_log_button_, &QPushButton::clicked, this,
            &MainWindow::OnClearLogClicked);

    // 协力自动控制
    connect(multi_start_button_, &QPushButton::clicked, this,
            &MainWindow::OnMultiPlayStartClicked);
    connect(multi_stop_button_, &QPushButton::clicked, this,
            &MainWindow::OnMultiPlayStopClicked);
}

void MainWindow::OnMumuPathButtonClicked() {
    QString dir = QFileDialog::getExistingDirectory(
        this, "选择模拟器目录", mumu_path_edit_->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        mumu_path_edit_->setText(dir);
    }
}

PlayConfig MainWindow::GetPlayConfig() const {
    PlayConfig config{};
    config.hold_cnt = 6;
    config.color_delta = color_delta_spin_->value();
    config.tap_duration_ms = tap_duration_spin_->value();
    config.check_loop_delay_ms = sample_freq_spin_->value() / 1000;
    config.hit_delay_ms = hit_delay_spin_->value();
    return config;
}

void MainWindow::InitAutoPlayer() {
    mumu_client_ = std::make_unique<MumuClient>(
        mumu_path_edit_->text(), mumu_inst_spin_->value(),
        MumuClient::kDefaultPackageName);

    note_estimator_ = std::make_unique<NoteTimeEstimator>(
        kDefaultTrackConfig.check_upper_y, kDefaultTrackConfig.check_lower_y);

    TrackConfig track_config = kDefaultTrackConfig;

    auto_player_ = std::make_unique<AutoPlayer>(
        *mumu_client_, *mumu_client_, *mumu_client_, *mumu_client_,
        *note_estimator_, track_config, GetPlayConfig());

    // 设置画面显示开关
    auto_player_->SetImgShowFlag(img_show_checkbox_->isChecked());

    // 设置画面显示关闭回调，用于同步复选框状态
    auto_player_->SetImgShowOffCallback([this]() {
        QMetaObject::invokeMethod(
            this, [this]() { img_show_checkbox_->setChecked(false); },
            Qt::QueuedConnection);
    });

    // 设置AutoPlayer的回调函数，用于同步按钮状态
    auto_player_->SetPlayStartCallback([this]() {
        // 在UI线程中更新按钮状态
        QMetaObject::invokeMethod(
            this,
            [this]() {
                if (!auto_check_enabled_checkbox_->isChecked()) {
                    start_button_->setEnabled(false);
                    stop_button_->setEnabled(true);
                    auto_check_enabled_checkbox_->setEnabled(false);
                    status_label_->setText("运行中...");
                }
            },
            Qt::QueuedConnection);
    });

    auto_player_->SetPlayStopCallback([this]() {
        QMetaObject::invokeMethod(
            this,
            [this]() {
                if (!auto_check_enabled_checkbox_->isChecked()) {
                    start_button_->setEnabled(true);
                    stop_button_->setEnabled(false);
                    auto_check_enabled_checkbox_->setEnabled(true);
                    status_label_->setText("就绪");
                }
            },
            Qt::QueuedConnection);
    });

    auto_player_->SetCheckStartCallback([this]() {
        QMetaObject::invokeMethod(
            this,
            [this]() {
                start_button_->setEnabled(false);
                stop_button_->setEnabled(true);
                auto_check_enabled_checkbox_->setChecked(true);
                auto_check_enabled_checkbox_->setEnabled(false);
                status_label_->setText("自动检测中...");
            },
            Qt::QueuedConnection);
    });

    auto_player_->SetCheckStopCallback([this]() {
        QMetaObject::invokeMethod(
            this,
            [this]() {
                start_button_->setEnabled(true);
                stop_button_->setEnabled(false);
                auto_check_enabled_checkbox_->setEnabled(true);
                status_label_->setText("就绪");
            },
            Qt::QueuedConnection);
    });
}

void MainWindow::OnStartButtonClicked() {
    try {
        if (!auto_player_) {
            InitAutoPlayer();
        } else {
            auto_player_->SetPlayConfig(GetPlayConfig());
            auto_player_->SetImgShowFlag(img_show_checkbox_->isChecked());
        }

        if (auto_check_enabled_checkbox_->isChecked()) {
            if (!auto_player_->AutoCheckStart()) {
                spdlog::warn("自动检测启动失败");
            }
        } else {
            if (!auto_player_->Start()) {
                throw std::runtime_error("自动播放器启动失败");
            }
        }

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "错误",
                              QString("启动失败: %1").arg(e.what()));
    }
}

void MainWindow::OnStopButtonClicked() {
    try {
        if (auto_player_) {
            if (auto_check_enabled_checkbox_->isChecked()) {
                auto_player_->AutoCheckStop();
            } else {
                auto_player_->Stop();
            }
        }

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "错误",
                              QString("停止失败: %1").arg(e.what()));
    }
}

void MainWindow::AddLogMessage(const QString& message) {
    // 添加时间戳（如果消息中没有的话）
    log_display_->append(message.trimmed());

    // 自动滚动到底部
    QScrollBar* scrollBar = log_display_->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::OnLogLevelChanged() {
    QString level = log_level_combo_->currentText();
    spdlog::level::level_enum spdlog_level = spdlog::level::info;

    if (level == "Debug") {
        spdlog_level = spdlog::level::debug;
    } else if (level == "Info") {
        spdlog_level = spdlog::level::info;
    } else if (level == "Warn") {
        spdlog_level = spdlog::level::warn;
    } else if (level == "Error") {
        spdlog_level = spdlog::level::err;
    }

    spdlog::set_level(spdlog_level);
    spdlog::info("日志级别已更改为: {}", level.toStdString());
}

void MainWindow::OnClearLogClicked() {
    log_display_->clear();
    spdlog::info("日志已清除");
}

void MainWindow::OnMultiPlayStartClicked() {
    try {
        if (!auto_player_) {
            InitAutoPlayer();
        }
        if (!multi_controller_) {
            multi_controller_ = std::make_unique<MultiPlayController>(
                *mumu_client_, *mumu_client_, *auto_player_);
        }

        // 设置MultiPlayController的回调函数，用于同步按钮状态
        multi_controller_->SetStartCallback([this]() {
            QMetaObject::invokeMethod(
                this,
                [this]() {
                    multi_start_button_->setEnabled(false);
                    multi_stop_button_->setEnabled(true);
                    status_label_->setText("协力运行中...");
                },
                Qt::QueuedConnection);
        });

        multi_controller_->SetStopCallback([this]() {
            QMetaObject::invokeMethod(
                this,
                [this]() {
                    multi_start_button_->setEnabled(true);
                    multi_stop_button_->setEnabled(false);
                    status_label_->setText("就绪");
                },
                Qt::QueuedConnection);
        });

        using Mode = MultiPlayController::PlayMode;
        using Diff = SongDiffculty;

        Mode mode = Mode::kFixedDifficulty;
        switch (multi_mode_combo_->currentIndex()) {
            case 0: mode = Mode::kFixedDifficulty; break;
            case 1: mode = Mode::kUnclearFirst; break;
            case 2: mode = Mode::kUnfullComboFirst; break;
            case 3: mode = Mode::kUnallPerfectFirst; break;
        }

        Diff max_diff = Diff::kMaster;
        switch (multi_max_diff_combo_->currentIndex()) {
            case 0: max_diff = Diff::kEasy; break;
            case 1: max_diff = Diff::kNormal; break;
            case 2: max_diff = Diff::kHard; break;
            case 3: max_diff = Diff::kExpert; break;
            case 4: max_diff = Diff::kMaster; break;
            case 5: max_diff = Diff::kAppend; break;
        }

        if (!multi_controller_->Start(mode, max_diff)) {
            throw std::runtime_error("协力自动控制启动失败");
        }

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "错误",
                              QString("协力启动失败: %1").arg(e.what()));
    }
}

void MainWindow::OnMultiPlayStopClicked() {
    try {
        if (multi_controller_) {
            multi_controller_->Stop();
        }

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "错误",
                              QString("协力停止失败: %1").arg(e.what()));
    }
}

} // namespace psh
