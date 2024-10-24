#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "customtablemodel.h"

#include <QMainWindow>
#include <QtSql>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    bool connectToAWSDatabase();
    void fetchDataAndInsertToLocalDB();
    QString selectedColumn();
    QString selectedDate();
    void on_showGraph_clicked();
    void on_refreshDataButton_clicked();
    void displayData();
    void on_InsertDataPagebutton_clicked();
    void on_pushButtonSend_clicked();
    void updateSelectedColumn(int index);
    void on_showAverage_clicked();
    void on_pushButtonHome_clicked();

private:
    Ui::MainWindow *ui;
    QSqlDatabase dbAWS;
    QSqlDatabase dbLocal;
    CustomTableModel *customModel;
    int lastFetchedID; // Variable to store the last fetched ID
    QStringList ColumnNames;
    int selectedColumnIndex;
};

#endif // MAINWINDOW_H
