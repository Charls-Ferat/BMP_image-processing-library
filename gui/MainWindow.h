#pragma once

#include <QList>
#include <QMainWindow>
#include <QString>

#include "ImageProcessor.h"

class QCheckBox;
class QCloseEvent;
class QColor;
class QComboBox;
class QDragEnterEvent;
class QDropEvent;
class QGroupBox;
class QLabel;
class QLayout;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTableWidget;
class QThread;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void chooseImages();
    void chooseOutputFolder();
    void previewSelected();
    void removeSelected();
    void clearQueue();
    void processQueue();
    void openOutputFolder();
    void onProcessingFinished();
    void updateControls();
    void updateExample();
    void showSelectedMessage();

private:
    enum Column { FileColumn, ModeColumn, StatusColumn, MessageColumn };

    struct TransformCheck {
        Transformation transformation;
        QCheckBox* box;
    };

    QLayout* createQueuePanel();
    QWidget* createSettingsPanel();

    void addFiles(const QStringList& paths);
    void addRow(const QString& path, const QString& problem);
    bool isRowValid(int row) const;
    int validRowCount() const;
    void setRowStatus(int row, const QString& status, const QString& message,
                      const QColor& color);
    void showResult(int row, const ProcessingResult& result);
    ProcessingOptions currentOptions() const;

    // Queue
    QStackedWidget* queueStack = nullptr;
    QTableWidget* table = nullptr;
    QPushButton* addButton = nullptr;
    QPushButton* previewButton = nullptr;
    QPushButton* removeButton = nullptr;
    QPushButton* clearButton = nullptr;

    // Settings
    QGroupBox* transformGroup = nullptr;
    QGroupBox* parametersGroup = nullptr;
    QGroupBox* outputGroup = nullptr;
    QList<TransformCheck> transformChecks;
    QCheckBox* blurCheck = nullptr;
    QSpinBox* blurSpin = nullptr;
    QComboBox* modeCombo = nullptr;
    QLineEdit* outputEdit = nullptr;
    QLineEdit* suffixEdit = nullptr;
    QLabel* exampleLabel = nullptr;
    QPushButton* processButton = nullptr;
    QPushButton* openFolderButton = nullptr;
    QLabel* timeLabel = nullptr;

    // Processing runs on a background thread so the window stays responsive;
    // inside it, OpenMP processes several images at the same time
    QThread* worker = nullptr;
    double elapsedSeconds = 0;      // written by the worker, read after it finishes
    int threadsUsed = 1;
    int totalJobs = 0;
    int savedCount = 0;
    int failedCount = 0;

    QString lastInputDir;
};
