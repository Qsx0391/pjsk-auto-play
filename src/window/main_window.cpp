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
#include <QScrollArea>
#include <QTimer>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "player/display_manager.h"

namespace {

template <typename Tuple, std::size_t... I>
void addLabelAndComboImpl(QBoxLayout* parent, Tuple&& tup,
                          std::index_sequence<I...>) {
    auto addOne = [parent](const QString& text, QComboBox* combo) {
        auto* hlayout = new QHBoxLayout;
        hlayout->setSpacing(0);
        hlayout->setContentsMargins(0, 0, 0, 0);

        auto* label = new QLabel(text);
        hlayout->addWidget(label);
        hlayout->addWidget(combo, 1);

        parent->addLayout(hlayout);
    };

    (addOne(std::get<2 * I>(tup), std::get<2 * I + 1>(tup)), ...);
}

template <typename... Args>
void addLabelAndCombo(QBoxLayout* parent, Args&&... args) {
    static_assert(sizeof...(args) % 2 == 0, "参数必须成对出现: label, combo");
    auto tup = std::make_tuple(std::forward<Args>(args)...);
    constexpr size_t N = sizeof...(args) / 2;
    addLabelAndComboImpl(parent, tup, std::make_index_sequence<N>{});
}

} // namespace

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

    // 连接 SusLoader 信号
    QObject::connect(
        &psh::SusLoader::Instance(), &psh::SusLoader::LoadStarted, this,
        [this]() {
            QMetaObject::invokeMethod(
                this, [this]() { sus_title_label_->setText("已加载: -"); },
                Qt::QueuedConnection);
        });
    QObject::connect(
        &psh::SusLoader::Instance(), &psh::SusLoader::SusUpdated, this,
        [this](std::shared_ptr<psh::SusLoader::SusResult> r) {
            QMetaObject::invokeMethod(
                this,
                [this, r]() {
                    sus_title_label_->setText("已加载: " + r->title +
                                              " 难度: " + r->difficulty);
                },
                Qt::QueuedConnection);
        });
    QObject::connect(
        &psh::SusLoader::Instance(), &psh::SusLoader::MatchesUpdated, this,
        [this](const std::vector<std::pair<SusLoader::SongInfo, double>>&
                   matches) {
            QMetaObject::invokeMethod(
                this,
                [this, matches]() {
                    sus_matches_cache_ = matches;
                    sus_matches_combo_->clear();
                    for (const auto& [song, sim] : matches) {
                        sus_matches_combo_->addItem(song.title);
                    }
                    sus_match_diff_combo_->clear();
                    if (!matches.empty()) {
                        for (const auto& d :
                             matches.front().first.difficulties) {
                            sus_match_diff_combo_->addItem(d);
                        }
                    }
                },
                Qt::QueuedConnection);
        });
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* event) {
    if (auto_player_) {
        auto_player_->Stop();
    }
    if (story_reader_) {
        story_reader_->Stop();
    }
    SaveSettings();
    event->accept();
}

void MainWindow::LoadSettings() {
    QSettings settings("PJSKAutoPlay");

    mumu_path_edit_->setText(
        settings.value("mumu/path", kDefaultMumuPath).toString());
    mumu_inst_spin_->setValue(
        settings.value("mumu/instance", kDefaultMumuInstance).toInt());

    const auto& pc = PlayConfig{};
    cv_hit_delay_spin_->setValue(
        settings.value("play/cv_hit_delay", pc.cv_hit_delay_ms).toInt());
    sus_hit_delay_spin_->setValue(
        settings.value("play/sus_hit_delay", pc.sus_hit_delay_ms).toInt());
    sample_freq_spin_->setValue(
        settings.value("play/sample_freq", 1000 / pc.check_loop_delay_ms)
            .toInt());

    img_show_checkbox_->setChecked(false);
    sus_mode_checkbox_->setChecked(
        settings.value("play/sus_mode", pc.sus_mode).toBool());
    speed_factor_combo_->setCurrentText(
        settings.value("play/speed_factor", "10.00").toString());
    play_mode_combo_->setCurrentIndex(settings.value("play/mode", 0).toInt());

    multi_mode_combo_->setCurrentIndex(settings.value("multi/mode", 0).toInt());
    multi_max_diff_combo_->setCurrentIndex(
        settings.value("multi/max_diff", 4).toInt());
}

void MainWindow::SaveSettings() {
    QSettings settings("PJSKAutoPlay");

    settings.setValue("mumu/path", mumu_path_edit_->text());
    settings.setValue("mumu/instance", mumu_inst_spin_->value());

    settings.setValue("play/cv_hit_delay", cv_hit_delay_spin_->value());
    settings.setValue("play/sus_hit_delay", sus_hit_delay_spin_->value());
    settings.setValue("play/sample_freq", sample_freq_spin_->value());
    settings.setValue("play/speed_factor", speed_factor_combo_->currentText());
    settings.setValue("play/sus_mode", sus_mode_checkbox_->isChecked());
    settings.setValue("play/mode", play_mode_combo_->currentIndex());

    settings.setValue("multi/mode", multi_mode_combo_->currentIndex());
    settings.setValue("multi/max_diff", multi_max_diff_combo_->currentIndex());
}

void MainWindow::SetupUi() {
    central_widget_ = new QWidget(this);
    setCentralWidget(central_widget_);

    main_splitter_ = new QSplitter(Qt::Horizontal, central_widget_);
    auto main_layout = new QHBoxLayout(central_widget_);
    main_layout->setContentsMargins(8, 8, 8, 8);
    main_layout->addWidget(main_splitter_);

    // ================= 左侧 =================
    auto left_scroll = new QScrollArea();
    left_scroll->setWidgetResizable(true);
    auto left_container = new QWidget();
    auto left_layout = new QVBoxLayout(left_container);
    left_layout->setSpacing(12); // group 之间留空隙
    left_scroll->setWidget(left_container);

    // ---- 模拟器设置 ----
    auto mumu_group = new QGroupBox("mumu模拟器设置");
    auto mumu_layout = new QFormLayout(mumu_group);
    mumu_layout->setLabelAlignment(Qt::AlignRight);

    auto mumu_path_layout = new QHBoxLayout();
    mumu_path_edit_ =
        new QLineEdit("E:/Program Files/Netease/MuMu Player 12", this);
    mumu_path_button_ = new QPushButton("浏览...", this);
    mumu_path_layout->addWidget(mumu_path_edit_);
    mumu_path_layout->addWidget(mumu_path_button_);
    mumu_layout->addRow("安装路径：", mumu_path_layout);

    mumu_inst_spin_ = new QSpinBox(this);
    mumu_inst_spin_->setRange(0, 1024);
    mumu_layout->addRow("实例编号：", mumu_inst_spin_);
    left_layout->addWidget(mumu_group);

    // ---- 自动打歌 ----
    auto config_group = new QGroupBox("自动打歌");
    auto config_layout = new QFormLayout(config_group);
    config_layout->setLabelAlignment(Qt::AlignRight);

    cv_hit_delay_spin_ = new QSpinBox(this);
    cv_hit_delay_spin_->setRange(-1000, 1000);
    config_layout->addRow("CV 点击延迟(ms)：", cv_hit_delay_spin_);

    sus_hit_delay_spin_ = new QSpinBox(this);
    sus_hit_delay_spin_->setRange(-1000, 1000);
    config_layout->addRow("SUS 点击延迟(ms)：", sus_hit_delay_spin_);

    sample_freq_spin_ = new QSpinBox(this);
    sample_freq_spin_->setRange(1, 1000);
    sample_freq_spin_->setValue(50);
    config_layout->addRow("检查频率(Hz)：", sample_freq_spin_);

    speed_factor_combo_ = new QComboBox(this);
    speed_factor_combo_->addItems({"6.00", "8.00", "9.00", "10.00", "11.00"});
    config_layout->addRow("流速：", speed_factor_combo_);

    play_mode_combo_ = new QComboBox(this);
    play_mode_combo_->addItems({"单次", "单人模式", "协力模式"});
    config_layout->addRow("游戏模式：", play_mode_combo_);

    auto config_checkbox_layout = new QHBoxLayout();
    sus_mode_checkbox_ = new QCheckBox("启用 SUS 模式", this);
    img_show_checkbox_ = new QCheckBox("显示画面", this);
    config_checkbox_layout->addWidget(sus_mode_checkbox_);
    config_checkbox_layout->addWidget(img_show_checkbox_);
    config_checkbox_layout->addStretch();
    config_layout->addRow(config_checkbox_layout);

    auto start_stop_layout = new QHBoxLayout();
    start_button_ = new QPushButton("开始", this);
    stop_button_ = new QPushButton("停止", this);
    stop_button_->setEnabled(false);
    start_stop_layout->addWidget(start_button_);
    start_stop_layout->addWidget(stop_button_);
    config_layout->addRow(start_stop_layout);

    left_layout->addWidget(config_group);

    // ---- 协力控制 ----
    auto multi_group = new QGroupBox("协力控制");
    auto multi_layout = new QHBoxLayout(multi_group);
    multi_mode_combo_ = new QComboBox(this);
    multi_mode_combo_->addItems({"固定难度", "自动选择"});
    multi_max_diff_combo_ = new QComboBox(this);
    multi_max_diff_combo_->addItems(
        {"Easy", "Normal", "Hard", "Expert", "Master", "Append"});
    multi_max_diff_combo_->setCurrentIndex(2);
    addLabelAndCombo(multi_layout, "难度选择：", multi_mode_combo_,
                     "最大难度：", multi_max_diff_combo_);
    left_layout->addWidget(multi_group);

    // ---- 自动阅读 ----
    auto story_group = new QGroupBox("自动阅读");
    auto story_layout = new QHBoxLayout(story_group);
    story_start_button_ = new QPushButton("开始", this);
    story_stop_button_ = new QPushButton("停止", this);
    story_stop_button_->setEnabled(false);
    story_layout->addWidget(story_start_button_);
    story_layout->addWidget(story_stop_button_);
    left_layout->addWidget(story_group);

    // ---- SUS 加载 ----
    auto sus_group = new QGroupBox("SUS 加载");
    auto sus_layout = new QVBoxLayout(sus_group);
    sus_title_label_ = new QLabel("已加载: -", this);
    // 手动输入歌名
    auto sus_name_layout = new QHBoxLayout();
    sus_name_edit_ = new QLineEdit(this);
    sus_name_edit_->setPlaceholderText("请输入歌曲名...");
    sus_load_by_name_button_ = new QPushButton("搜索", this);
    sus_name_layout->addWidget(sus_name_edit_, 3);
    sus_name_layout->addWidget(sus_load_by_name_button_, 1);
    sus_layout->addLayout(sus_name_layout);
    sus_matches_combo_ = new QComboBox(this);
    sus_match_diff_combo_ = new QComboBox(this);
    sus_reload_button_ = new QPushButton("重新加载", this);
    sus_refresh_songs_button_ = new QPushButton("刷新曲库缓存", this);

    auto sus_controls_layout = new QHBoxLayout();
    sus_controls_layout->addWidget(new QLabel("候选："));
    sus_controls_layout->addWidget(sus_matches_combo_, 3);
    sus_controls_layout->addWidget(new QLabel("难度："));
    sus_controls_layout->addWidget(sus_match_diff_combo_, 1);
    sus_layout->addLayout(sus_controls_layout);

    sus_layout->addWidget(sus_title_label_);

    auto sus_buttons_layout = new QHBoxLayout();
    sus_buttons_layout->addWidget(sus_refresh_songs_button_);
    sus_buttons_layout->addWidget(sus_reload_button_);
    sus_layout->addLayout(sus_buttons_layout);

    left_layout->addWidget(sus_group);

    // ---- 状态 ----
    status_label_ = new QLabel("状态: 未开始", this);
    left_layout->addWidget(status_label_);
    left_layout->addStretch();

    // ================= 右侧日志 =================
    auto right_widget = new QWidget();
    auto right_layout = new QVBoxLayout(right_widget);

    auto log_control_layout = new QHBoxLayout();
    log_control_layout->addWidget(new QLabel("日志级别"));
    log_level_combo_ = new QComboBox(this);
    log_level_combo_->addItems({"Debug", "Info", "Warn", "Error"});
    log_level_combo_->setCurrentText("Info");
    clear_log_button_ = new QPushButton("清除日志", this);
    log_control_layout->addWidget(log_level_combo_);
    log_control_layout->addStretch();
    log_control_layout->addWidget(clear_log_button_);
    right_layout->addLayout(log_control_layout);

    log_display_ = new QPlainTextEdit(this);
    log_display_->setReadOnly(true);
    log_display_->setFont(QFont("Consolas", 9));
    right_layout->addWidget(log_display_, 1);

    // ================= 拼接到主分割器 =================
    main_splitter_->addWidget(left_scroll);
    main_splitter_->addWidget(right_widget);
    main_splitter_->setStretchFactor(0, 0);
    main_splitter_->setStretchFactor(1, 1);
    main_splitter_->setSizes({400, 600});

    setWindowTitle("PJSK Auto Play");
    setMinimumSize(900, 600);
}

void MainWindow::CreateConnections() {
    connect(start_button_, &QPushButton::clicked, this,
            &MainWindow::OnStartButtonClicked);
    connect(stop_button_, &QPushButton::clicked, this,
            &MainWindow::OnStopButtonClicked);
    connect(story_start_button_, &QPushButton::clicked, this,
            &MainWindow::OnStoryStartButtonClicked);
    connect(story_stop_button_, &QPushButton::clicked, this,
            &MainWindow::OnStoryStopButtonClicked);
    connect(mumu_path_button_, &QPushButton::clicked, this,
            &MainWindow::OnMumuPathButtonClicked);
    connect(sus_mode_checkbox_, &QCheckBox::toggled, this,
            [this](bool checked) {});

    connect(img_show_checkbox_, &QCheckBox::toggled, this,
            &MainWindow::OnImgShowChanged);

    connect(log_level_combo_,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::OnLogLevelChanged);
    connect(clear_log_button_, &QPushButton::clicked, this,
            &MainWindow::OnClearLogClicked);


    connect(sus_reload_button_, &QPushButton::clicked, this, [this]() {
        int idx = sus_matches_combo_->currentIndex();
        if (idx < 0 || idx >= static_cast<int>(sus_matches_cache_.size()))
            return;
        const auto& song = sus_matches_cache_[idx];
        const QString diff = sus_match_diff_combo_->currentText();
        psh::SusLoader::LoadSusByName(song.first.title, diff, false, true);
    });

    connect(sus_refresh_songs_button_, &QPushButton::clicked, this, [this]() {
        status_label_->setText("状态: 正在刷新曲库缓存...");
        psh::SusLoader::RefreshSongsCache();
        QTimer::singleShot(1000, this,
                           [this]() { status_label_->setText("就绪"); });
    });

    // 手动按歌名加载：按钮点击与回车
    connect(sus_load_by_name_button_, &QPushButton::clicked, this,
            &MainWindow::OnSusLoadByNameClicked);
    connect(sus_name_edit_, &QLineEdit::returnPressed, this,
            &MainWindow::OnSusLoadByNameClicked);
}

void MainWindow::OnMumuPathButtonClicked() {
    QString dir = QFileDialog::getExistingDirectory(
        this, "选择模拟器目录", mumu_path_edit_->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        mumu_path_edit_->setText(dir);
    }
}

void MainWindow::OnImgShowChanged(bool checked) {
    if (auto_player_) {
        if (checked) {
            DisplayManager::StartDisplay();
        } else {
            DisplayManager::StopDisplay();
        }
    }
}

PlayConfig MainWindow::GetPlayConfig() const {
    PlayConfig pc{};
    pc.hold_cnt = 6;
    pc.check_loop_delay_ms = 1000 / sample_freq_spin_->value();
    pc.cv_hit_delay_ms = cv_hit_delay_spin_->value();
    pc.sus_hit_delay_ms = sus_hit_delay_spin_->value();
    pc.speed_factor = GetCurrentSpeedFactor();
    pc.sus_mode = sus_mode_checkbox_->isChecked();
    pc.auto_select = multi_mode_combo_->currentIndex() == 1;

    switch (multi_max_diff_combo_->currentIndex()) {
        case 0: pc.max_diff = SongDifficulty::kEasy; break;
        case 1: pc.max_diff = SongDifficulty::kNormal; break;
        case 2: pc.max_diff = SongDifficulty::kHard; break;
        case 3: pc.max_diff = SongDifficulty::kExpert; break;
        case 4: pc.max_diff = SongDifficulty::kMaster; break;
        case 5: pc.max_diff = SongDifficulty::kAppend; break;
    }

    return pc;
}

SpeedFactor MainWindow::GetCurrentSpeedFactor() const {
    QString speed_text = speed_factor_combo_->currentText();
    if (speed_text == "6.00") {
        return SpeedFactor::kSpeed6x;
    } else if (speed_text == "8.00") {
        return SpeedFactor::kSpeed8x;
    } else if (speed_text == "9.00") {
        return SpeedFactor::kSpeed9x;
    } else if (speed_text == "10.00") {
        return SpeedFactor::kSpeed10x;
    } else if (speed_text == "11.00") {
        return SpeedFactor::kSpeed11x;
    }
    return SpeedFactor::kSpeed10x;
}

PlayMode MainWindow::GetCurrentPlayMode() const {
    int index = play_mode_combo_->currentIndex();
    switch (index) {
        case 0: return PlayMode::kOnce;
        case 1: return PlayMode::kSolo;
        case 2: return PlayMode::kMulti;
        default: return PlayMode::kOnce;
    }
}

void MainWindow::InitAutoPlayer() {
    if (auto_player_) {
        auto_player_->SetPlayConfig(GetPlayConfig());
        auto_player_->SetPlayMode(GetCurrentPlayMode());
    } else {
        InitMumuClient();

        auto_player_ = std::make_unique<AutoPlayer>(
            *touch_controller_, *mumu_client_, TrackConfig{}, GetPlayConfig());

        QObject::connect(auto_player_.get(), &AutoPlayer::playStopped, this,
                         [this]() {
                             QMetaObject::invokeMethod(
                                 this,
                                 [this]() {
                                     UpdateUiOnStopped();
                                 },
                                 Qt::QueuedConnection);
                         });

        auto_player_->SetPlayMode(GetCurrentPlayMode());

        QObject::connect(
            &DisplayManager::Instance(), &DisplayManager::displayOff, this,
            [this]() {
                QMetaObject::invokeMethod(
                    this, [this]() { img_show_checkbox_->setChecked(false); },
                    Qt::QueuedConnection);
            });
    }
}

void MainWindow::InitMumuClient() {
    if (!mumu_client_) {
        mumu_client_ = std::make_unique<MumuClient>(
            mumu_path_edit_->text(), mumu_inst_spin_->value(),
            MumuClient::kDefaultPackageName);
        touch_controller_ = std::make_unique<TouchController>(*mumu_client_);
    }
}

void MainWindow::InitStoryReader() {
    if (!story_reader_) {
        InitMumuClient();
        story_reader_ = std::make_unique<StoryAutoReader>(*touch_controller_);

        QObject::connect(story_reader_.get(), &StoryAutoReader::stopped, this,
                         [this]() {
                             QMetaObject::invokeMethod(
                                 this,
                                 [this]() {
                                     story_start_button_->setEnabled(true);
                                     story_stop_button_->setEnabled(false);
                                 },
                                 Qt::QueuedConnection);
                         });
    }
}

void MainWindow::UpdateUiOnStarted() {
    start_button_->setEnabled(false);
    stop_button_->setEnabled(true);
    status_label_->setText("运行中...");
}

void MainWindow::UpdateUiOnStopped() {
    start_button_->setEnabled(true);
    stop_button_->setEnabled(false);
    status_label_->setText("就绪");
}

void MainWindow::OnStartButtonClicked() {
    try {
        InitAutoPlayer();
        if (!auto_player_->Start()) {
            throw std::runtime_error("自动打歌启动失败");
        }
        UpdateUiOnStarted();
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
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "错误",
                              QString("停止失败: %1").arg(e.what()));
    }
}

void MainWindow::AddLogMessage(const QString& message) {
    // 添加时间戳（如果消息中没有的话）
    log_display_->appendPlainText(message.trimmed());

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

void MainWindow::OnStoryStartButtonClicked() {
    try {
        InitStoryReader();
        if (!story_reader_->Start()) {
            throw std::runtime_error("故事自动阅读启动失败");
        }
        // 启动成功后更新按钮状态
        story_start_button_->setEnabled(false);
        story_stop_button_->setEnabled(true);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "错误",
                              QString("启动失败: %1").arg(e.what()));
    }
}

void MainWindow::OnStoryStopButtonClicked() {
    try {
        if (story_reader_) {
            story_reader_->Stop();
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "错误",
                              QString("停止失败: %1").arg(e.what()));
    }
}

void MainWindow::OnSusLoadByNameClicked() {
    const QString song = sus_name_edit_->text().trimmed();
    if (song.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入歌名");
        return;
    }
    status_label_->setText("状态: 正在按歌名加载...");
    psh::SusLoader::LoadSusByName(song, "", false, false);
}

} // namespace psh
