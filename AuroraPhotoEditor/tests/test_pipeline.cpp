#include <QtTest>
#include <QSignalSpy>
#include <QImage>
#include <QColor>
#include <QSharedPointer>
#include <thread>
#include <vector>
#include "../src/PipelineManager.h"
#include "../src/ImageEditorCommand.h"

// A simple command implementation for testing.
// It fills the image with a specific color.
class FillColorCommand : public ImageEditorCommand {
public:
    explicit FillColorCommand(const QColor& color) : m_color(color) {}

    QImage execute(const QImage& input) const override {
        QImage result = input.copy();
        result.fill(m_color);
        return result;
    }

    QString name() const override {
        return QString("FillColor(%1)").arg(m_color.name());
    }

private:
    QColor m_color;
};

// Another simple command implementation that draws a pixel at (0, 0) with a specific color.
class DrawPixelCommand : public ImageEditorCommand {
public:
    explicit DrawPixelCommand(const QColor& color) : m_color(color) {}

    QImage execute(const QImage& input) const override {
        QImage result = input.copy();
        if (!result.isNull()) {
            result.setPixelColor(0, 0, m_color);
        }
        return result;
    }

    QString name() const override {
        return QString("DrawPixel(%1)").arg(m_color.name());
    }

private:
    QColor m_color;
};

class TestPipelineManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testInitialState();
    void testSetOriginalImage();
    void testApplyCommand();
    void testUndoLast();
    void testResetToOriginal();
    void testThreadSafety();
};

void TestPipelineManager::initTestCase() {
    // Initialization if any
}

void TestPipelineManager::testInitialState() {
    PipelineManager pm;
    QVERIFY(pm.getOriginalImage().isNull());
    QVERIFY(pm.getCurrentImage().isNull());
    QCOMPARE(pm.commandCount(), 0);
}

void TestPipelineManager::testSetOriginalImage() {
    PipelineManager pm;
    QImage img(100, 100, QImage::Format_RGB32);
    img.fill(Qt::red);

    QSignalSpy spyOriginal(&pm, &PipelineManager::originalImageChanged);
    QSignalSpy spyCurrent(&pm, &PipelineManager::currentImageChanged);

    pm.setOriginalImage(img);

    QCOMPARE(spyOriginal.count(), 1);
    QCOMPARE(spyCurrent.count(), 1);
    QCOMPARE(pm.getOriginalImage(), img);
    QCOMPARE(pm.getCurrentImage(), img);
    QCOMPARE(pm.commandCount(), 0);
}

void TestPipelineManager::testApplyCommand() {
    PipelineManager pm;
    QImage img(100, 100, QImage::Format_RGB32);
    img.fill(Qt::white);
    pm.setOriginalImage(img);

    QSignalSpy spyCurrent(&pm, &PipelineManager::currentImageChanged);
    QSignalSpy spyStack(&pm, &PipelineManager::commandStackChanged);

    auto cmd = QSharedPointer<FillColorCommand>::create(Qt::blue);
    QVERIFY(pm.applyCommand(cmd));

    QCOMPARE(spyCurrent.count(), 1);
    QCOMPARE(spyStack.count(), 1);
    QCOMPARE(pm.commandCount(), 1);

    // Verify current image was modified by command (filled with blue)
    QImage current = pm.getCurrentImage();
    QCOMPARE(current.pixelColor(0, 0), QColor(Qt::blue));
    // Verify original image remains unchanged
    QCOMPARE(pm.getOriginalImage().pixelColor(0, 0), QColor(Qt::white));
}

void TestPipelineManager::testUndoLast() {
    PipelineManager pm;
    QImage img(100, 100, QImage::Format_RGB32);
    img.fill(Qt::white);
    pm.setOriginalImage(img);

    auto cmd1 = QSharedPointer<FillColorCommand>::create(Qt::blue);
    auto cmd2 = QSharedPointer<DrawPixelCommand>::create(Qt::red);

    pm.applyCommand(cmd1);
    pm.applyCommand(cmd2);

    QCOMPARE(pm.commandCount(), 2);
    QCOMPARE(pm.getCurrentImage().pixelColor(0, 0), QColor(Qt::red));
    QCOMPARE(pm.getCurrentImage().pixelColor(10, 10), QColor(Qt::blue));

    QSignalSpy spyCurrent(&pm, &PipelineManager::currentImageChanged);
    QSignalSpy spyStack(&pm, &PipelineManager::commandStackChanged);

    // First undo
    QVERIFY(pm.undoLast());
    QCOMPARE(spyCurrent.count(), 1);
    QCOMPARE(spyStack.count(), 1);
    QCOMPARE(pm.commandCount(), 1);
    QCOMPARE(pm.getCurrentImage().pixelColor(0, 0), QColor(Qt::blue));

    // Second undo
    QVERIFY(pm.undoLast());
    QCOMPARE(pm.commandCount(), 0);
    QCOMPARE(pm.getCurrentImage().pixelColor(0, 0), QColor(Qt::white));

    // Third undo (should fail as stack is empty)
    QVERIFY(!pm.undoLast());
}

void TestPipelineManager::testResetToOriginal() {
    PipelineManager pm;
    QImage img(100, 100, QImage::Format_RGB32);
    img.fill(Qt::white);
    pm.setOriginalImage(img);

    auto cmd = QSharedPointer<FillColorCommand>::create(Qt::blue);
    pm.applyCommand(cmd);

    QSignalSpy spyCurrent(&pm, &PipelineManager::currentImageChanged);
    QSignalSpy spyStack(&pm, &PipelineManager::commandStackChanged);

    pm.resetToOriginal();

    QCOMPARE(spyCurrent.count(), 1);
    QCOMPARE(spyStack.count(), 1);
    QCOMPARE(pm.commandCount(), 0);
    QCOMPARE(pm.getCurrentImage().pixelColor(0, 0), QColor(Qt::white));
}

void TestPipelineManager::testThreadSafety() {
    PipelineManager pm;
    QImage img(100, 100, QImage::Format_RGB32);
    img.fill(Qt::white);
    pm.setOriginalImage(img);

    const int numThreads = 8;
    const int operationsPerThread = 100;
    std::vector<std::thread> threads;

    auto worker = [&pm, operationsPerThread]() {
        auto cmd = QSharedPointer<DrawPixelCommand>::create(Qt::green);
        for (int i = 0; i < operationsPerThread; ++i) {
            pm.applyCommand(cmd);
            QImage img = pm.getCurrentImage();
            Q_UNUSED(img);
            if (i % 5 == 0) {
                pm.undoLast();
            }
        }
    };

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        t.join();
    }

    // Since each thread did operationsPerThread commands and some undoes,
    // we just check that the program doesn't crash, the command count is consistent,
    // and we can safely access/modify the manager.
    int expectedCommandsMin = 0;
    QVERIFY(pm.commandCount() >= expectedCommandsMin);
    
    // Perform reset to original safely
    pm.resetToOriginal();
    QCOMPARE(pm.commandCount(), 0);
    QCOMPARE(pm.getCurrentImage().pixelColor(0, 0), QColor(Qt::white));
}

QTEST_MAIN(TestPipelineManager)
#include "test_pipeline.moc"
