#pragma once

#include <memory>

#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QFileDialog>
#include <QSettings>
#include <QCloseEvent>
#include <QCheckBox>
#include <QTextEdit>
#include <QComboBox>
#include <QSplitter>
#include <QPlainTextEdit>

#include "player/auto_player.h"
#include "mumu/mumu_client.h"
#include "player/note_time_estimator.h"
#include "player/note_finder.h"
#include "story/story_auto_reader.h"
#include "sus/sus_loader.h"

namespace psh {
class QtLogSink;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

public slots:
    void AddLogMessage(const QString &message);

private slots:
    void OnStartButtonClicked();
    void OnStopButtonClicked();
    void OnMumuPathButtonClicked();
    void OnImgShowChanged(bool checked);
    void OnLogLevelChanged();
    void OnClearLogClicked();
    void OnStoryStartButtonClicked();
    void OnStoryStopButtonClicked();
    void OnSusLoadByNameClicked();

private:
    inline static const QString kDefaultMumuPath = "";
    static const int kDefaultMumuInstance = 0;
    static const int kDefaultAdbPort = 16384;
    inline static const QString kDefaultPackageName = "default";

    void SetupUi();
    void CreateConnections();
    void InitAutoPlayer();
    void InitMumuClient();
    void InitStoryReader();
    void LoadSettings();
    void SaveSettings();
    void UpdateUiOnStarted();
    void UpdateUiOnStopped();
    PlayConfig GetPlayConfig() const;
    SpeedFactor GetCurrentSpeedFactor() const;
    PlayMode GetCurrentPlayMode() const;

    // UI elements
    QWidget *central_widget_;
    QSplitter *main_splitter_;

    // Auto Play
    QSpinBox *cv_hit_delay_spin_;
    QSpinBox *sus_hit_delay_spin_;
    QSpinBox *sample_freq_spin_;
    QComboBox *speed_factor_combo_;
    QComboBox *play_mode_combo_;
    QCheckBox *img_show_checkbox_;
    QCheckBox *sus_mode_checkbox_;
    QPushButton *start_button_;
    QPushButton *stop_button_;

    // Log display controls
    QPlainTextEdit *log_display_;
    QComboBox *log_level_combo_;
    QPushButton *clear_log_button_;

    // Mumu settings
    QLineEdit *mumu_path_edit_;
    QPushButton *mumu_path_button_;
    QSpinBox *mumu_inst_spin_;

    // Multi play controls
    QComboBox *multi_mode_combo_;
    QComboBox *multi_max_diff_combo_;

    // Story auto reader controls
    QPushButton *story_start_button_;
    QPushButton *story_stop_button_;

    // SUS info and reload controls
    QLabel *sus_title_label_;
    QLineEdit *sus_name_edit_;
    QComboBox *sus_matches_combo_;
    QComboBox *sus_match_diff_combo_;
    QPushButton *sus_reload_button_;
    QPushButton *sus_load_by_name_button_;
    QPushButton *sus_refresh_songs_button_;

    // Status label
    QLabel *status_label_;

    // Core components
    std::unique_ptr<AutoPlayer> auto_player_;
    std::unique_ptr<MumuClient> mumu_client_;
    std::unique_ptr<TouchController> touch_controller_;
    std::unique_ptr<StoryAutoReader> story_reader_;

    // cache matches
    std::vector<std::pair<SusLoader::SongInfo, double>> sus_matches_cache_;
};

} // namespace psh