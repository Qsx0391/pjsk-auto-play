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

#include "auto_player.h"
#include "mumu_client.h"
#include "mini_touch_client.h"
#include "note_time_estimator.h"
#include "note_finder.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void OnStartButtonClicked();
    void OnStopButtonClicked();
    void OnMumuPathButtonClicked();

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
    QPushButton *start_button_;
    QPushButton *stop_button_;
    QLabel *status_label_;

    // Mumu settings
    QLineEdit *mumu_path_edit_;
    QPushButton *mumu_path_button_;
    QSpinBox *mumu_inst_spin_;
    QSpinBox *adb_port_spin_;
    QLineEdit *package_name_edit_;

    // Play settings
    QSpinBox *color_delta_spin_;
    QSpinBox *hit_delay_spin_;
    QSpinBox *tap_duration_spin_;
    QSpinBox *check_loop_delay_spin_;

    // Core components
    std::unique_ptr<psh::AutoPlayer> auto_player_;
    std::unique_ptr<psh::MumuClient> mumu_client_;
    std::unique_ptr<psh::MiniTouchClient> mini_touch_client_;
    std::unique_ptr<psh::NoteTimeEstimator> note_estimator_;
};