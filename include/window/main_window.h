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

#include "auto_player.h"
#include "mumu_client.h"
#include "mini_touch_client.h"
#include "note_time_estimator.h"
#include "note_finder.h"
#include "multi_play_controller.h"

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
    void OnLogLevelChanged();
    void OnClearLogClicked();
    void OnMultiPlayStartClicked();
    void OnMultiPlayStopClicked();

private:
    inline static const QString kDefaultMumuPath = "";
    static const int kDefaultMumuInstance = 0;
    static const int kDefaultAdbPort = 16384;
    inline static const QString kDefaultPackageName = "default";

    void SetupUi();
    void CreateConnections();
    void InitAutoPlayer();
    void LoadSettings();
    void SaveSettings();
    psh::PlayConfig GetPlayConfig() const;

    // UI elements
    QWidget *central_widget_;
    QSplitter *main_splitter_;
    QPushButton *start_button_;
    QPushButton *stop_button_;
    QLabel *status_label_;

    // Auto check controls
    QCheckBox *auto_check_enabled_checkbox_;

    // Log display controls
    QTextEdit *log_display_;
    QComboBox *log_level_combo_;
    QPushButton *clear_log_button_;

    // Mumu settings
    QLineEdit *mumu_path_edit_;
    QPushButton *mumu_path_button_;
    QSpinBox *mumu_inst_spin_;
    // QSpinBox *adb_port_spin_;
    // QLineEdit *package_name_edit_;

    // Play settings
    QSpinBox *color_delta_spin_;
    QSpinBox *hit_delay_spin_;
    QSpinBox *tap_duration_spin_;
    QSpinBox *sample_freq_spin_;
    QCheckBox *img_show_checkbox_;

    // Multi play controls
    QComboBox *multi_mode_combo_;
    QComboBox *multi_max_diff_combo_;
    QPushButton *multi_start_button_;
    QPushButton *multi_stop_button_;

    // Core components
    std::unique_ptr<psh::AutoPlayer> auto_player_;
    std::unique_ptr<psh::MumuClient> mumu_client_;
    std::unique_ptr<psh::MiniTouchClient> mini_touch_client_;
    std::unique_ptr<psh::NoteTimeEstimator> note_estimator_;
    std::unique_ptr<psh::MultiPlayController> multi_controller_;
};

} // namespace psh