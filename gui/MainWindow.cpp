#include "MainWindow.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QColor>
#include <QComboBox>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QScreen>
#include <QSet>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QThread>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

namespace {

const QColor kBlue(0x25, 0x63, 0xeb);
const QColor kGreen(0x15, 0x80, 0x3d);
const QColor kRed(0xb9, 0x1c, 0x1c);
const QColor kGray(0x47, 0x55, 0x69);

const char* const kStyleSheet = R"(
QWidget { background: #f3f6fb; color: #1e293b; font-size: 13px; }
QLabel, QCheckBox { background: transparent; }
QLabel#banner { background: #2563eb; color: #ffffff; font-size: 18px; font-weight: 600; padding: 12px 18px; }
QLabel#muted { color: #64748b; font-size: 12px; }
QLabel#dropHint { background: #eaf1ff; color: #1d4ed8; border: 2px dashed #93b4f5; border-radius: 8px; font-size: 16px; }
QLabel#preview { background: #e6ecf6; }

QGroupBox { background: #ffffff; border: 1px solid #d5deec; border-radius: 6px; margin-top: 12px; padding: 14px 10px 10px 10px; font-weight: 600; }
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; color: #1d4ed8; background: #f3f6fb; }

QPushButton { background: #ffffff; color: #1e3a8a; border: 1px solid #b6c4dc; border-radius: 5px; padding: 6px 14px; }
QPushButton:hover { background: #e8f0fe; }
QPushButton:pressed { background: #d6e4fd; }
QPushButton:disabled { background: #eef2f8; color: #94a3b8; border-color: #dbe3f0; }
QPushButton#primary { background: #2563eb; color: #ffffff; border: 1px solid #1d4ed8; font-weight: 600; }
QPushButton#primary:hover { background: #1d4ed8; }
QPushButton#primary:pressed { background: #1e40af; }
QPushButton#primary:disabled { background: #9db8f0; border-color: #9db8f0; color: #ffffff; }

QLineEdit, QSpinBox, QComboBox { background: #ffffff; border: 1px solid #b6c4dc; border-radius: 4px; padding: 4px 6px; selection-background-color: #2563eb; selection-color: #ffffff; }
QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border: 1px solid #2563eb; }
QLineEdit:disabled, QSpinBox:disabled, QComboBox:disabled { background: #eef2f8; color: #94a3b8; }
QComboBox QAbstractItemView { background: #ffffff; selection-background-color: #2563eb; selection-color: #ffffff; }

QTableWidget { background: #ffffff; alternate-background-color: #f7f9fd; border: 1px solid #d5deec; border-radius: 6px; selection-background-color: #dbe7fd; selection-color: #1e3a8a; }
QTableWidget::item { padding: 4px; }
QHeaderView::section { background: #2563eb; color: #ffffff; padding: 6px; border: none; font-weight: 600; }

QStatusBar { background: #ffffff; border-top: 1px solid #d5deec; }
QToolTip { background: #1e3a8a; color: #ffffff; border: none; padding: 4px; }
)";

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("BMP Image Processor");
    resize(1040, 640);
    setMinimumSize(880, 540);
    setAcceptDrops(true);
    setStyleSheet(kStyleSheet);

    auto* central = new QWidget;
    auto* outer = new QVBoxLayout(central);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* banner = new QLabel("BMP Image Processor");
    banner->setObjectName("banner");
    outer->addWidget(banner);

    auto* body = new QHBoxLayout;
    body->setContentsMargins(16, 16, 16, 16);
    body->setSpacing(16);
    body->addLayout(createQueuePanel(), 1);
    body->addWidget(createSettingsPanel());
    outer->addLayout(body, 1);

    setCentralWidget(central);
    statusBar()->showMessage("Drag and drop BMP files into the window to start.");
    updateControls();
}

QLayout* MainWindow::createQueuePanel() {
    auto* layout = new QVBoxLayout;

    // Empty state and queue share the same space
    queueStack = new QStackedWidget;

    auto* hint = new QLabel("Drag and drop BMP images here\nor use \"Add Images\"");
    hint->setObjectName("dropHint");
    hint->setAlignment(Qt::AlignCenter);
    queueStack->addWidget(hint);

    table = new QTableWidget(0, 4);
    table->setHorizontalHeaderLabels({"File", "Color mode", "Status", "Message"});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->verticalHeader()->hide();
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(ModeColumn, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(StatusColumn, QHeaderView::ResizeToContents);
    table->setColumnWidth(FileColumn, 220);
    queueStack->addWidget(table);

    layout->addWidget(queueStack, 1);

    auto* buttons = new QHBoxLayout;
    addButton = new QPushButton("Add Images...");
    previewButton = new QPushButton("Preview Original");
    removeButton = new QPushButton("Remove Selected");
    removeButton->setShortcut(QKeySequence::Delete);
    clearButton = new QPushButton("Clear Queue");
    buttons->addWidget(addButton);
    buttons->addWidget(previewButton);
    buttons->addStretch();
    buttons->addWidget(removeButton);
    buttons->addWidget(clearButton);
    layout->addLayout(buttons);

    connect(addButton, &QPushButton::clicked, this, &MainWindow::chooseImages);
    connect(previewButton, &QPushButton::clicked, this, &MainWindow::previewSelected);
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::removeSelected);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearQueue);
    connect(table, &QTableWidget::cellDoubleClicked, this, &MainWindow::previewSelected);
    connect(table, &QTableWidget::itemSelectionChanged, this, &MainWindow::updateControls);
    connect(table, &QTableWidget::itemSelectionChanged, this, &MainWindow::showSelectedMessage);

    return layout;
}

QWidget* MainWindow::createSettingsPanel() {
    auto* panel = new QWidget;
    panel->setFixedWidth(310);
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    // Transformations
    transformGroup = new QGroupBox("Transformations");
    auto* transformLayout = new QVBoxLayout(transformGroup);
    for (Transformation t : {Transformation::FlipHorizontal, Transformation::FlipVertical,
                             Transformation::Blur, Transformation::DerivativeX,
                             Transformation::DerivativeY}) {
        auto* box = new QCheckBox(ImageProcessor::transformationName(t));
        transformLayout->addWidget(box);
        transformChecks.append(TransformCheck{t, box});
        if (t == Transformation::Blur) {
            blurCheck = box;
        }
    }
    auto* orderLabel = new QLabel("Selected transformations are applied in the order listed.");
    orderLabel->setObjectName("muted");
    orderLabel->setWordWrap(true);
    transformLayout->addWidget(orderLabel);
    layout->addWidget(transformGroup);

    // Parameters
    parametersGroup = new QGroupBox("Parameters");
    auto* form = new QFormLayout(parametersGroup);

    blurSpin = new QSpinBox;
    blurSpin->setRange(1, 999);
    blurSpin->setValue(5);
    blurSpin->setToolTip("Box blur size in pixels. The library uses a radius of size / 2, "
                         "so an even size gives the same result as the next odd size, "
                         "and 1 leaves the image unchanged.");
    form->addRow("Blur kernel size", blurSpin);

    modeCombo = new QComboBox;
    modeCombo->addItem(ImageProcessor::colorModeName(ColorMode::Rgb));
    modeCombo->addItem(ImageProcessor::colorModeName(ColorMode::Grayscale));
    modeCombo->setToolTip("Color mode of the saved image. The library converts it as part "
                          "of the last selected transformation.");
    form->addRow("Output color mode", modeCombo);
    layout->addWidget(parametersGroup);

    // Output
    outputGroup = new QGroupBox("Output");
    auto* outputLayout = new QVBoxLayout(outputGroup);

    outputLayout->addWidget(new QLabel("Destination folder"));
    auto* folderRow = new QHBoxLayout;
    outputEdit = new QLineEdit;
    outputEdit->setPlaceholderText("Choose a folder...");
    auto* browseButton = new QPushButton("Browse...");
    folderRow->addWidget(outputEdit, 1);
    folderRow->addWidget(browseButton);
    outputLayout->addLayout(folderRow);

    outputLayout->addWidget(new QLabel("Filename suffix"));
    suffixEdit = new QLineEdit("_processed");
    suffixEdit->setValidator(
        new QRegularExpressionValidator(QRegularExpression("[^\\\\/:*?\"<>|]*"), suffixEdit));
    outputLayout->addWidget(suffixEdit);

    exampleLabel = new QLabel;
    exampleLabel->setObjectName("muted");
    outputLayout->addWidget(exampleLabel);

    auto* noOverwriteLabel = new QLabel("Existing files are never overwritten. If a name is "
                                        "already taken, a number is added to it.");
    noOverwriteLabel->setObjectName("muted");
    noOverwriteLabel->setWordWrap(true);
    outputLayout->addWidget(noOverwriteLabel);
    layout->addWidget(outputGroup);

    // Actions
    processButton = new QPushButton("Process and Save");
    processButton->setObjectName("primary");
    processButton->setMinimumHeight(38);
    openFolderButton = new QPushButton("Open Output Folder");
    layout->addWidget(processButton);
    layout->addWidget(openFolderButton);
    layout->addStretch();

    connect(browseButton, &QPushButton::clicked, this, &MainWindow::chooseOutputFolder);
    connect(processButton, &QPushButton::clicked, this, &MainWindow::processQueue);
    connect(openFolderButton, &QPushButton::clicked, this, &MainWindow::openOutputFolder);
    connect(blurCheck, &QCheckBox::toggled, this, &MainWindow::updateControls);
    connect(outputEdit, &QLineEdit::textChanged, this, &MainWindow::updateControls);
    connect(suffixEdit, &QLineEdit::textChanged, this, &MainWindow::updateExample);
    updateExample();

    return panel;
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (!worker && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    QStringList paths;
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            paths << url.toLocalFile();
        }
    }
    addFiles(paths);
    event->acceptProposedAction();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Let the running job finish instead of killing the thread mid-write
    if (worker) {
        statusBar()->showMessage("Waiting for the current processing to finish...");
        worker->wait();
    }
    event->accept();
}

void MainWindow::chooseImages() {
    QStringList files = QFileDialog::getOpenFileNames(
        this, "Select BMP images", lastInputDir,
        "BMP images (*.bmp *.BMP);;All files (*)");
    if (!files.isEmpty()) {
        addFiles(files);
    }
}

void MainWindow::chooseOutputFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select output folder",
                                                    outputEdit->text());
    if (!dir.isEmpty()) {
        outputEdit->setText(dir);
    }
}

void MainWindow::addFiles(const QStringList& paths) {
    QSet<QString> queued;
    for (int row = 0; row < table->rowCount(); ++row) {
        queued.insert(table->item(row, FileColumn)->data(Qt::UserRole).toString());
    }

    int added = 0;
    int unsupported = 0;
    int duplicates = 0;
    int notFiles = 0;
    QString firstAdded;

    for (const QString& path : paths) {
        QFileInfo info(path);
        if (!info.isFile()) {
            ++notFiles;
            continue;
        }

        QString fullPath = info.absoluteFilePath();
        if (queued.contains(fullPath)) {
            ++duplicates;
            continue;
        }
        queued.insert(fullPath);

        QString problem = ImageProcessor::validateBmp(fullPath);
        addRow(fullPath, problem);
        if (firstAdded.isEmpty()) {
            firstAdded = fullPath;
        }
        ++added;
        if (!problem.isEmpty()) {
            ++unsupported;
        }
    }

    if (!firstAdded.isEmpty()) {
        lastInputDir = QFileInfo(firstAdded).absolutePath();
        // Suggest the folder of the first image until the user picks another one
        if (outputEdit->text().trimmed().isEmpty()) {
            outputEdit->setText(lastInputDir);
        }
    }

    QStringList summary;
    summary << QString("Added %1 file(s)").arg(added);
    if (unsupported > 0) {
        summary << QString("%1 unsupported (see the Message column)").arg(unsupported);
    }
    if (duplicates > 0) {
        summary << QString("%1 already in the queue").arg(duplicates);
    }
    if (notFiles > 0) {
        summary << QString("%1 item(s) skipped because they are not files").arg(notFiles);
    }
    statusBar()->showMessage(summary.join(", ") + ".");
    updateControls();
}

void MainWindow::addRow(const QString& path, const QString& problem) {
    bool valid = problem.isEmpty();
    int row = table->rowCount();
    table->insertRow(row);

    auto* fileItem = new QTableWidgetItem(QFileInfo(path).fileName());
    fileItem->setData(Qt::UserRole, path);
    fileItem->setData(Qt::UserRole + 1, valid);
    fileItem->setToolTip(path);
    table->setItem(row, FileColumn, fileItem);

    // bmp_read() always loads the image as RGB
    table->setItem(row, ModeColumn,
                   new QTableWidgetItem(valid ? ImageProcessor::colorModeName(ColorMode::Rgb)
                                              : QString("-")));
    table->setItem(row, StatusColumn, new QTableWidgetItem);
    table->setItem(row, MessageColumn, new QTableWidgetItem);

    if (valid) {
        setRowStatus(row, "Queued", QString(), kGray);
    } else {
        setRowStatus(row, "Unsupported", problem, kRed);
    }
}

bool MainWindow::isRowValid(int row) const {
    return table->item(row, FileColumn)->data(Qt::UserRole + 1).toBool();
}

int MainWindow::validRowCount() const {
    int count = 0;
    for (int row = 0; row < table->rowCount(); ++row) {
        if (isRowValid(row)) {
            ++count;
        }
    }
    return count;
}

void MainWindow::setRowStatus(int row, const QString& status, const QString& message,
                              const QColor& color) {
    QTableWidgetItem* statusItem = table->item(row, StatusColumn);
    statusItem->setText(status);
    statusItem->setForeground(color);

    QTableWidgetItem* messageItem = table->item(row, MessageColumn);
    messageItem->setText(message);
    messageItem->setToolTip(message);
    messageItem->setForeground(color);
}

void MainWindow::showResult(int row, const ProcessingResult& result) {
    if (result.ok) {
        ++savedCount;
        table->item(row, ModeColumn)->setText(ImageProcessor::colorModeName(result.mode));
        setRowStatus(row, "Done",
                     "Saved as " + QFileInfo(result.outputPath).fileName(), kGreen);
    } else {
        ++failedCount;
        setRowStatus(row, "Failed", result.error, kRed);
    }
    statusBar()->showMessage(QString("Processed %1 of %2...")
                                 .arg(savedCount + failedCount).arg(totalJobs));
}

void MainWindow::showSelectedMessage() {
    QModelIndexList rows = table->selectionModel()->selectedRows();
    if (rows.size() != 1) {
        return;
    }
    int row = rows.first().row();
    QString message = table->item(row, MessageColumn)->text();
    if (!message.isEmpty()) {
        statusBar()->showMessage(table->item(row, FileColumn)->text() + ": " + message);
    }
}

void MainWindow::previewSelected() {
    QModelIndexList rows = table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return;
    }
    QString path = table->item(rows.first().row(), FileColumn)->data(Qt::UserRole).toString();

    QImage image(path);
    if (image.isNull()) {
        QMessageBox::warning(this, "Preview",
                             "The image could not be opened for the preview:\n" + path);
        return;
    }

    QPixmap pixmap = QPixmap::fromImage(image);
    QSize limit = screen()->availableGeometry().size() * 0.8;
    if (pixmap.width() > limit.width() || pixmap.height() > limit.height()) {
        pixmap = pixmap.scaled(limit, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    auto* window = new QLabel(this, Qt::Window);
    window->setObjectName("preview");
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->setWindowTitle(QString("%1 (%2 x %3)")
                               .arg(QFileInfo(path).fileName())
                               .arg(image.width()).arg(image.height()));
    window->setPixmap(pixmap);
    window->show();
}

void MainWindow::removeSelected() {
    QList<int> rows;
    for (const QModelIndex& index : table->selectionModel()->selectedRows()) {
        rows << index.row();
    }
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int row : rows) {
        table->removeRow(row);
    }
    statusBar()->showMessage(QString("Removed %1 file(s) from the queue.").arg(rows.size()));
    updateControls();
}

void MainWindow::clearQueue() {
    table->setRowCount(0);
    statusBar()->showMessage("Queue cleared.");
    updateControls();
}

ProcessingOptions MainWindow::currentOptions() const {
    ProcessingOptions options;
    for (const TransformCheck& check : transformChecks) {
        if (check.box->isChecked()) {
            options.transformations.append(check.transformation);
        }
    }
    options.outputMode = (modeCombo->currentIndex() == 1) ? ColorMode::Grayscale
                                                          : ColorMode::Rgb;
    options.blurKernelSize = blurSpin->value();
    return options;
}

void MainWindow::processQueue() {
    ProcessingOptions options = currentOptions();
    if (options.transformations.isEmpty()) {
        QMessageBox::warning(this, "No transformation",
                             "Select at least one transformation before processing.");
        return;
    }

    QString outputDir = outputEdit->text().trimmed();
    QFileInfo dirInfo(outputDir);
    if (outputDir.isEmpty() || !dirInfo.isDir() || !dirInfo.isWritable()) {
        QMessageBox::warning(this, "Output folder",
                             "Choose an existing output folder that you can write to.");
        return;
    }

    struct Job {
        int row;
        QString path;
    };
    QList<Job> jobs;
    for (int row = 0; row < table->rowCount(); ++row) {
        if (!isRowValid(row)) {
            continue;
        }
        // Clear the result of any previous run
        table->item(row, ModeColumn)->setText(ImageProcessor::colorModeName(ColorMode::Rgb));
        setRowStatus(row, "Queued", QString(), kGray);
        jobs.append(Job{row, table->item(row, FileColumn)->data(Qt::UserRole).toString()});
    }
    if (jobs.isEmpty()) {
        return;
    }

    QString suffix = suffixEdit->text();
    totalJobs = static_cast<int>(jobs.size());
    savedCount = 0;
    failedCount = 0;

    // Images are processed one after another; a failure only affects its own
    // row. Results go back to the UI thread through queued calls.
    worker = QThread::create([this, jobs, outputDir, suffix, options]() {
        for (const Job& job : jobs) {
            int row = job.row;
            QMetaObject::invokeMethod(this, [this, row]() {
                setRowStatus(row, "Processing", QString(), kBlue);
            }, Qt::QueuedConnection);

            ProcessingResult result =
                ImageProcessor::process(job.path, outputDir, suffix, options);

            QMetaObject::invokeMethod(this, [this, row, result]() {
                showResult(row, result);
            }, Qt::QueuedConnection);
        }
    });
    worker->setParent(this);
    connect(worker, &QThread::finished, this, &MainWindow::onProcessingFinished);
    worker->start();

    statusBar()->showMessage(QString("Processing %1 image(s)...").arg(totalJobs));
    updateControls();
}

void MainWindow::onProcessingFinished() {
    worker->deleteLater();
    worker = nullptr;

    QString message = QString("Finished: %1 saved, %2 failed.").arg(savedCount).arg(failedCount);
    if (failedCount > 0) {
        message += " Select a failed row to read the error.";
    }
    statusBar()->showMessage(message);
    updateControls();
}

void MainWindow::openOutputFolder() {
    QString dir = outputEdit->text().trimmed();
    if (!QFileInfo(dir).isDir()) {
        QMessageBox::warning(this, "Output folder", "The output folder does not exist yet.");
        return;
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(dir))) {
        QMessageBox::warning(this, "Output folder",
                             "The system could not open the folder:\n" + dir);
    }
}

void MainWindow::updateControls() {
    bool busy = (worker != nullptr);
    bool hasRows = table->rowCount() > 0;
    bool hasSelection = !table->selectionModel()->selectedRows().isEmpty();

    queueStack->setCurrentIndex(hasRows ? 1 : 0);

    addButton->setEnabled(!busy);
    previewButton->setEnabled(hasSelection);
    removeButton->setEnabled(!busy && hasSelection);
    clearButton->setEnabled(!busy && hasRows);
    processButton->setEnabled(!busy && validRowCount() > 0);
    openFolderButton->setEnabled(!outputEdit->text().trimmed().isEmpty());

    transformGroup->setEnabled(!busy);
    parametersGroup->setEnabled(!busy);
    outputGroup->setEnabled(!busy);
    blurSpin->setEnabled(blurCheck->isChecked());
}

void MainWindow::updateExample() {
    exampleLabel->setText("Example: photo" + suffixEdit->text() + ".bmp");
}
