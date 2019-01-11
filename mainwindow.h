#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFile>
#include <QFileDialog>
#include <QMainWindow>
#include <QMap>
#include <QTableWidget>
#include <QTextStream>
#include <QTime>
#include <QVector>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

  private slots:
    void on_startButton_clicked();

  private:
    Ui::MainWindow *ui;

    void openDirectory();
    void CleanMemory();
    void CleanFeatures();
    void Exit();

    void Normalize(int, int, QVector<QVector<QVector<int>>> &);
    void initializeConfussionMatrix(int);
    void showConfussionMatrix();
    void cleanConfussionMatrix();
    void resetConfussionMatrix();
    double jaccard_yule(short);
    void projections(short);
    void zones(short);
    double classify();

    // recursive subdivisions utils
    void subdivisions(short);
    int find_index(QVector<int>);
    int find_vertical_point(QVector<QVector<int>>);
    int find_horizontal_point(QVector<QVector<int>>);
    void recursive_mock(int, int, int);
    void recursive_div(QVector<QVector<int>>, int, int, int);

    int maxWidth = -999;
    int maxHeight = -999;

    // raw image values
    QVector<QVector<QVector<int>>> train_images;
    QVector<QVector<QVector<int>>> test_images;

    // image class and features
    QVector<QVector<double>> trainset;
    QVector<QVector<double>> testset;

    // class map (classId -> className)
    QMap<int, QString> class_map;
    QMap<int, int> class_count_map;

    // confussion matrix
    QTableWidget *uiConfussionMatrix = nullptr;
    QVector<QVector<int>> confMatrix;
};

#endif // MAINWINDOW_H
