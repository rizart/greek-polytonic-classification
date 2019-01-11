#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <math.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    // sender, signal, receiver, slot
    connect(ui->actionOpen, &QAction::triggered, this,
            &MainWindow::openDirectory);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::Exit);
}

MainWindow::~MainWindow() {
    delete ui;
}

// -----------------------------------------------------------------------------

void MainWindow::openDirectory() {
    // get main directory path
    QString mainDirPathString =
        QFileDialog::getExistingDirectory(this, tr("Open Directory"));
    if (mainDirPathString.isNull())
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    CleanMemory();

    // get sub-directories
    QDir mainDir(mainDirPathString);
    QStringList subDirs = mainDir.entryList();

    ui->textBrowser->append("Loading images.. ");
    ui->textBrowser->moveCursor(QTextCursor::End);

    int classId = 0;
    int trainCount = 0;
    int testCount = 0;

    // foreach class directory
    for (auto const &subDir : subDirs) {
        if (subDir == "." || subDir == "..")
            continue;

        // get sub-directory path
        QString subDirPath = mainDirPathString + "/" + subDir;

        // keep class mapping
        class_map[classId] = subDir;
        class_count_map[classId] = 0;

        // get images in sub-directory
        QDir subDirD(subDirPath);
        QStringList imagesStrings = subDirD.entryList();

        QVector<QVector<QVector<int>>> *tVector;

        // foreach image in sub-directory (class directory)
        foreach (QString imgName, imagesStrings) {
            if (imgName == "." || imgName == "..")
                continue;

            // get image id
            int imgId =
                imgName.split(".", QString::SkipEmptyParts).at(0).toInt();

            // print image name and image number
            // QTextStream(stdout) << imgName << " | " << imgId << endl;

            // get image
            QFile *f = new QFile(subDirPath + "/" + imgName);
            QImage Image = QImage(f->fileName());

            int Ix = Image.width();
            int Iy = Image.height();
            int bpp = Image.depth();

            if (imgId % 2 == 0) {
                tVector = &test_images; // test set
                testset.resize(testset.size() + 1);
                testset[testCount].resize(1);
                testset[testCount][0] = classId;
                testCount++;
                class_count_map[classId]++;
            } else {
                tVector = &train_images; // train set
                trainset.resize(trainset.size() + 1);
                trainset[trainCount].resize(1);
                trainset[trainCount][0] = classId;
                trainCount++;
            }

            QVector<QVector<int>> img;
            img.resize(Iy);

            if (bpp == 1) // binary
            {
                for (int y = 0; y < Iy; y++) {
                    img[y].resize(Ix);
                    for (int x = 0; x < Ix; x++) {
                        if (Image.pixel(x, y) == qRgb(0, 0, 0))
                            img[y][x] = 1;
                        else
                            img[y][x] = 0;
                    }
                }
            } else {
                // QTextStream(stdout) << bpp << endl;
                ui->textBrowser->append("Wrong Input: Pictures must be binary");
                CleanMemory();
                QApplication::restoreOverrideCursor();
                return;
            }
            // add image to vector
            tVector->push_back(img);

            // keep max width,height
            if (Ix > maxWidth)
                maxWidth = Ix;
            if (Iy > maxHeight)
                maxHeight = Iy;
        }
        classId++;
    }
    ui->textBrowser->insertPlainText("DONE");

    QString information = "";
    //    information += "maxWidth:" + QString::number(maxWidth);
    //    information += "\nmaxHeight:" + QString::number(maxHeight);
    information += "Trainset size: " + QString::number(train_images.size());
    information += "\nTestset size:" + QString::number(test_images.size());
    ui->textBrowser->append(information);

    maxWidth = 50;
    maxHeight = 50;

    // normalize
    ui->textBrowser->append("Normalizing.. ");
    ui->textBrowser->moveCursor(QTextCursor::End);
    Normalize(maxWidth, maxHeight, train_images);
    Normalize(maxWidth, maxHeight, test_images);
    ui->textBrowser->insertPlainText("DONE");
    QApplication::restoreOverrideCursor();

    // setup confussion matrix
    int numOfClasses = class_map.size();
    uiConfussionMatrix = new QTableWidget(numOfClasses, numOfClasses);
    uiConfussionMatrix->showGrid();
    initializeConfussionMatrix(numOfClasses);
}

// -----------------------------------------------------------------------------

void MainWindow::resetConfussionMatrix() {
    uiConfussionMatrix->clearContents();
    for (int i = 0; i != confMatrix.size(); i++)
        for (int j = 0; j != confMatrix[i].size(); j++)
            confMatrix[i][j] = 0;
}

void MainWindow::initializeConfussionMatrix(int numOfClasses) {
    confMatrix.resize(numOfClasses);
    for (int i = 0; i != confMatrix.size(); i++) {
        confMatrix[i].resize(numOfClasses);
        for (int j = 0; j != confMatrix[i].size(); j++)
            confMatrix[i][j] = 0;
    }

    for (int i = 0; i != confMatrix.size(); i++) {
        uiConfussionMatrix->setHorizontalHeaderItem(
            i, new QTableWidgetItem(class_map[i]));
        uiConfussionMatrix->setVerticalHeaderItem(
            i, new QTableWidgetItem(class_map[i]));
    }
}

void MainWindow::showConfussionMatrix() {
    for (int i = 0; i != confMatrix.size(); i++) {
        for (int j = 0; j != confMatrix[i].size(); j++) {
            if (confMatrix[i][j] != 0 || i == j) {
                if (i == j) {
                    QString v = QString::number(confMatrix[i][j]) + "/" +
                                QString::number(class_count_map[i]);
                    uiConfussionMatrix->setItem(i, j, new QTableWidgetItem(v));
                } else
                    uiConfussionMatrix->setItem(
                        i, j,
                        new QTableWidgetItem(
                            QString::number(confMatrix[i][j])));
            }
        }
    }
    uiConfussionMatrix->setWindowTitle("Confussion Matrix");
    uiConfussionMatrix->show();
}

// -----------------------------------------------------------------------------

void MainWindow::Normalize(int maxWidth, int maxHeight,
                           QVector<QVector<QVector<int>>> &v) {
    for (int i = 0; i != v.size(); i++) {
        QVector<QVector<int>> *img = &v[i];
        int y = img->size();
        int x = (*img)[0].size();
        if (y < maxHeight) {
            img->resize(maxHeight);
            for (int yt = y; yt < maxHeight; yt++) {
                (*img)[yt].resize(x);
                for (int xt = 0; xt < x; xt++)
                    (*img)[yt][xt] = 0;
            }
            y = img->size();
        }
        if (x < maxWidth) {
            for (int yt = 0; yt < y; yt++) {
                (*img)[yt].resize(maxWidth);
                for (int xt = x; xt < maxWidth; xt++)
                    (*img)[yt][xt] = 0;
            }
        }
    }
}

// -----------------------------------------------------------------------------

void MainWindow::cleanConfussionMatrix() {
    confMatrix.clear();
    confMatrix.squeeze();
    if (uiConfussionMatrix != nullptr)
        delete uiConfussionMatrix;
}

void MainWindow::CleanMemory() {
    // clear confussion matrix
    cleanConfussionMatrix();

    // clear raw image values
    train_images.clear();
    test_images.clear();
    train_images.squeeze();
    test_images.squeeze();

    // clear image class and features
    trainset.clear();
    testset.clear();
    trainset.squeeze();
    testset.squeeze();

    // clear class map
    class_map.clear();
    class_count_map.clear();

    maxWidth = -999;
    maxHeight = -999;
}

void MainWindow::CleanFeatures() {
    for (int i = 0; i < trainset.size(); i++)
        trainset[i].resize(1);
    for (int i = 0; i < testset.size(); i++)
        testset[i].resize(1);
}

void MainWindow::Exit() {
    CleanMemory();
    QCoreApplication::exit(0);
}

// -----------------------------------------------------------------------------

void MainWindow::on_startButton_clicked() {
    if (!train_images.size() || !test_images.size()) {
        ui->textBrowser->append("No images loaded yet!");
        return;
    }

    resetConfussionMatrix();
    double accuracy = -3;
    QTime myTimer;
    myTimer.start();

    if (ui->jaccardButton->isChecked()) {
        ui->textBrowser->append("\nClassifying with jaccard distance..");
        QApplication::setOverrideCursor(Qt::WaitCursor);
        accuracy = jaccard_yule(0);
    }

    if (ui->yuleButton->isChecked()) {
        ui->textBrowser->append("\nClassifying with Yule distance..");
        QApplication::setOverrideCursor(Qt::WaitCursor);
        accuracy = jaccard_yule(1);
    }

    if (ui->projectionsButton->isChecked()) {
        CleanFeatures();
        ui->textBrowser->append("\nClassifying with " +
                                ui->comboBox->currentText() + " projections..");
        QApplication::setOverrideCursor(Qt::WaitCursor);
        projections(0);
        projections(1);
        accuracy = classify();
    }

    if (ui->zonesButton->isChecked()) {
        CleanFeatures();
        ui->textBrowser->append("\nClassifying with " +
                                ui->comboBox_2->currentText() + " zones..");
        QApplication::setOverrideCursor(Qt::WaitCursor);
        zones(0);
        zones(1);
        accuracy = classify();
    }

    if (ui->granButton->isChecked()) {
        CleanFeatures();
        int num_of_features =
            static_cast<int>(pow(4, ui->comboBox_4->currentText().toInt()));
        ui->textBrowser->append("\nClassifying with subdivisions.. [features=" +
                                QString::number(num_of_features) +
                                ", L=" + ui->comboBox_4->currentText() + "]");
        QApplication::setOverrideCursor(Qt::WaitCursor);
        subdivisions(0);
        subdivisions(1);
        accuracy = classify();
    }

    QApplication::restoreOverrideCursor();
    int ms = myTimer.elapsed();
    QString out = QString("%1:%2")
                      .arg(ms / 60000, 2, 10, QChar('0'))
                      .arg((ms % 60000) / 1000, 2, 10, QChar('0'));
    ui->textBrowser->insertPlainText(" (" + out + ")");
    ui->textBrowser->append("Accuracy = " + QString::number(accuracy) + "%");
    showConfussionMatrix();
}

// -----------------------------------------------------------------------------

double MainWindow::classify() {
    int correct = 0;

    // find euclidean distance from each pattern
    for (int k = 0; k < testset.size(); k++) {
        double mindist = 1000000;
        int cclass = -4;

        for (int i = 0; i < trainset.size(); i++) {
            // euclidean distance
            double distance = 0.0;
            for (int j = 1; j < trainset[i].size(); j++) {
                double td = trainset[i][j] - testset[k][j];
                td = td < 0 ? -td : td;
                distance += td;
            }

            if (distance < mindist) {
                mindist = distance;
                cclass = static_cast<int>(trainset[i][0]);
            }
        }

        if (cclass == testset[k][0])
            correct++;
        confMatrix[cclass][int(testset[k][0])]++;
    }
    return ((double)correct * 100) / testset.size();
}

// -----------------------------------------------------------------------------

int MainWindow::find_index(QVector<int> v1) {
    // prefix sum vector
    QVector<int> prefix_sum;
    prefix_sum.resize(v1.size());
    prefix_sum[0] = v1[0];
    for (int i = 1; i < v1.size(); i++)
        prefix_sum[i] = prefix_sum[i - 1] + v1[i];

    // suffix sum vector
    QVector<int> suffix_sum;
    suffix_sum.resize(v1.size());
    suffix_sum[v1.size() - 1] = v1[v1.size() - 1];
    for (int i = v1.size() - 2; i >= 0; i--)
        suffix_sum[i] = suffix_sum[i + 1] + v1[i];

    int min_index = -9;
    int min = 999999999;
    for (int i = 1; i < v1.size() - 1; i++)
        if (abs(prefix_sum[i] - suffix_sum[i]) < min) {
            min = abs(prefix_sum[i] - suffix_sum[i]);
            min_index = i;
        }
    return min_index;
}

int MainWindow::find_vertical_point(QVector<QVector<int>> img) {
    int image_height = img.size();
    int image_width = img[0].size();

    // initialize v0 and fill it with zeros
    QVector<int> v0;
    v0.resize(image_width);
    for (int m = 0; m < image_width; m++)
        v0[m] = 0;
    // foreach column of the image
    for (int x = 0; x < image_width; x++) {
        // foreach row of the image
        for (int y = 0; y < image_height; y++) {
            if (img[y][x]) {
                v0[x]++; // vertical pixels
            }
        }
    }

    // initialize v1 and populate it
    QVector<int> v1;
    v1.resize(image_width * 2);
    int l = 0;
    for (int m = 0; m < v1.size(); m++) {
        if (m % 2 == 0)
            v1[m] = 0;
        else {
            v1[m] = v0[l];
            l++;
        }
    }

    // find index that minimizes sum difference
    int Xq = find_index(v1);
    return Xq + 1;
}

int MainWindow::find_horizontal_point(QVector<QVector<int>> img) {
    int image_height = img.size();
    int image_width = img[0].size();

    // initialize v0 and fill it with zeros
    QVector<int> v0;
    v0.resize(image_height);
    for (int m = 0; m < image_height; m++)
        v0[m] = 0;
    // foreach row of the image
    for (int y = 0; y < image_height; y++) {
        // foreach column of the image
        for (int x = 0; x < image_width; x++) {
            if (img[y][x])
                v0[y]++; // horizontal pixels
        }
    }
    // initialize v1 and populate it
    QVector<int> v1;
    v1.resize(image_height * 2);
    int lh = 0;
    for (int m = 0; m < v1.size(); m++) {
        if (m % 2 == 0)
            v1[m] = 0;
        else {
            v1[m] = v0[lh];
            lh++;
        }
    }
    // find index that minimizes sum difference
    int Yq = find_index(v1);
    return Yq + 1;
}

void MainWindow::recursive_mock(int gran, int choice, int si) {
    if (gran > 0) {
        recursive_mock(gran - 1, choice, si);
        recursive_mock(gran - 1, choice, si);
        recursive_mock(gran - 1, choice, si);
        recursive_mock(gran - 1, choice, si);
    } else {
        if (!choice) {
            trainset[si].push_back(0);
            trainset[si].push_back(0);
        } else {
            testset[si].push_back(0);
            testset[si].push_back(0);
        }
    }
}

void MainWindow::recursive_div(QVector<QVector<int>> img, int gran, int choice,
                               int si) {
    int image_height = img.size();
    int image_width = img[0].size();

    // can't be split any further - just fill remaining features with (0,0)
    if (image_height < 3 || image_width < 3) {
        if (gran > 0) {
            recursive_mock(gran - 1, choice, si);
            recursive_mock(gran - 1, choice, si);
            recursive_mock(gran - 1, choice, si);
            recursive_mock(gran - 1, choice, si);
        } else {
            if (!choice) {
                trainset[si].push_back(0);
                trainset[si].push_back(0);
            } else {
                testset[si].push_back(0);
                testset[si].push_back(0);
            }
        }
        return;
    }

    int Xq = find_vertical_point(img);
    int X0 = Xq / 2;

    int Yq = find_horizontal_point(img);
    int Y0 = Yq / 2;

    // -------------------------------------------------------------------------

    // left-up sub-image
    QVector<QVector<int>> left_up_sub_img;
    left_up_sub_img.resize(Y0);
    for (int y = 0; y < Y0; y++) {
        left_up_sub_img[y].resize(X0);
        for (int x = 0; x < X0; x++) {
            left_up_sub_img[y][x] = img[y][x];
        }
    }

    // right-up sub-image
    QVector<QVector<int>> right_up_sub_img;
    right_up_sub_img.resize(Y0);
    int xfrom = -1;
    for (int y = 0; y < Y0; y++) {
        // --
        if (Xq % 2 == 0) {
            xfrom = X0 - 1;
            right_up_sub_img[y].resize(image_width - X0 + 1);
        } else {
            xfrom = X0;
            right_up_sub_img[y].resize(image_width - X0);
        }
        // --
        int xf = 0;
        for (int x = xfrom; x < image_width; x++) {
            right_up_sub_img[y][xf] = img[y][x];
            xf++;
        }
    }

    // left-down sub-image
    QVector<QVector<int>> left_down_sub_img;
    // --
    int yfrom = -1;
    if (Yq % 2 == 0) {
        yfrom = Y0 - 1;
        left_down_sub_img.resize(image_height - Y0 + 1);
    } else {
        yfrom = Y0;
        left_down_sub_img.resize(image_height - Y0);
    }
    // --
    int yf = 0;
    for (int y = yfrom; y < image_height; y++) {
        left_down_sub_img[yf].resize(X0);
        for (int x = 0; x < X0; x++)
            left_down_sub_img[yf][x] = img[y][x];
        yf++;
    }

    // right-down sub-image
    QVector<QVector<int>> right_down_sub_img;
    // --
    int yfrom2 = -1;
    if (Yq % 2 == 0) {
        yfrom2 = Y0 - 1;
        right_down_sub_img.resize(image_height - Y0 + 1);
    } else {
        yfrom2 = Y0;
        right_down_sub_img.resize(image_height - Y0);
    }
    // --
    int z2 = 0;
    int xfrom2 = -1;
    for (int y = yfrom2; y < image_height; y++) {
        // --
        if (Xq % 2 == 0) {
            xfrom2 = X0 - 1;
            right_down_sub_img[z2].resize(image_width - X0 + 1);
        } else {
            xfrom2 = X0;
            right_down_sub_img[z2].resize(image_width - X0);
        }
        // --
        int q = 0;
        for (int x = xfrom2; x < image_width; x++) {
            right_down_sub_img[z2][q] = img[y][x];
            q++;
        }
        z2++;
    }

    if (gran > 0) {
        recursive_div(left_up_sub_img, gran - 1, choice, si);
        recursive_div(right_up_sub_img, gran - 1, choice, si);
        recursive_div(left_down_sub_img, gran - 1, choice, si);
        recursive_div(right_down_sub_img, gran - 1, choice, si);
    } else {
        if (!choice) {
            trainset[si].push_back(X0);
            trainset[si].push_back(Y0);
        } else {
            testset[si].push_back(X0);
            testset[si].push_back(Y0);
        }
    }
}

void MainWindow::subdivisions(short choice) {
    // assign level of granularity for subdivisions
    int n = ui->comboBox_4->currentText().toInt();

    QVector<QVector<QVector<int>>> *choice_vector;

    if (!choice)
        choice_vector = &train_images;
    else
        choice_vector = &test_images;

    // for every image of the vector
    for (int m = 0; m < choice_vector->size(); m++) {
        QVector<QVector<int>> cur_img = (*choice_vector)[m]; // current image
        recursive_div(cur_img, n, choice, m);
    }
}

// -----------------------------------------------------------------------------

void MainWindow::projections(short choice) {
    // assign projections
    int n = ui->comboBox->currentText().toInt();

    QVector<QVector<QVector<int>>> *choice_vector;

    if (!choice)
        choice_vector = &train_images;
    else
        choice_vector = &test_images;

    // for every image of the vector
    for (int m = 0; m < choice_vector->size(); m++) {
        QVector<QVector<int>> cur_img = (*choice_vector)[m]; // current image
        // for every projection
        for (int k = 1; k <= n; k++) {
            int rpixels = 0;
            int cpixels = 0;
            // traverse pixels
            for (int y = 0; y < k * cur_img.size() / n; y++) {
                for (int x = 0; x < cur_img[0].size(); x++) {
                    if (cur_img[y][x])
                        rpixels++; // horizontal projections
                    if (cur_img[x][y])
                        cpixels++; // vertical projections
                }
            }

            if (!choice) {
                trainset[m].push_back((double)rpixels);
                trainset[m].push_back((double)cpixels);
            } else {
                testset[m].push_back((double)rpixels);
                testset[m].push_back((double)cpixels);
            }
        }
    }
}

// -----------------------------------------------------------------------------

void MainWindow::zones(short choice) {
    int p;
    if (ui->comboBox_2->currentText() == "2x2")
        p = 2;
    if (ui->comboBox_2->currentText() == "5x5")
        p = 5;
    if (ui->comboBox_2->currentText() == "10x10")
        p = 10;
    if (ui->comboBox_2->currentText() == "25x25")
        p = 25;

    QVector<QVector<QVector<int>>> *choice_vector;

    if (!choice)
        choice_vector = &train_images;
    else
        choice_vector = &test_images;

    // for ever image
    for (int m = 0; m < choice_vector->size(); m++) {
        QVector<QVector<int>> cur_img = (*choice_vector)[m]; // current image
        for (int k = 0; k < cur_img.size() / p; k++) {
            for (int l = 0; l < cur_img[0].size() / p; l++) {
                int pixels = 0;
                for (int y = k * p; y < ((k + 1) * p); y++) {
                    for (int x = l * p; x < ((l + 1) * p); x++) {
                        if (cur_img[y][x])
                            pixels++;
                    }
                }
                if (!choice)
                    trainset[m].push_back((double)pixels / (p * p));
                else
                    testset[m].push_back((double)pixels / (p * p));
            }
        }
    }
}

// -----------------------------------------------------------------------------

double MainWindow::jaccard_yule(short choice) {
    int correct = 0;

    // for every test image
    for (int i = 0; i != test_images.size(); i++) {
        double maxdist = -1000000;
        int cclass = -4;

        // for every train image
        for (int j = 0; j != train_images.size(); j++) {
            double n11 = 0;
            double n10 = 0;
            double n01 = 0;
            double n00 = 0;

            // traverse pixels
            for (int y = 0; y != train_images[0].size(); y++) {
                for (int x = 0; x != train_images[0][0].size(); x++) {
                    if (test_images[i][y][x] == 1 && train_images[j][y][x] == 1)
                        n11++;
                    else if (test_images[i][y][x] == 1 &&
                             train_images[j][y][x] == 0)
                        n10++;
                    else if (test_images[i][y][x] == 0 &&
                             train_images[j][y][x] == 0)
                        n00++;
                    else if (test_images[i][y][x] == 0 &&
                             train_images[j][y][x] == 1)
                        n01++;
                }
            }

            double distance;

            // jaccard
            if (choice == 0)
                distance = n11 / (n11 + n10 + n01);
            // yule
            else
                distance =
                    ((n11 * n00) - (n10 * n01)) / ((n11 * n00) + (n10 * n01));

            if (distance > maxdist) {
                maxdist = distance;
                cclass = trainset[j][0];
            }
        }

        if (cclass == testset[i][0])
            correct++;
        confMatrix[cclass][int(testset[i][0])]++;
    }
    return ((double)correct * 100) / testset.size();
}
