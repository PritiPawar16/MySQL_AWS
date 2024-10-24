#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->stackedWidget->setCurrentIndex(0);
    customModel = new CustomTableModel(this);
    ui->tableView->setModel(customModel);
    connect(ui->connectToDatabase,&QPushButton::clicked,this,&MainWindow::connectToAWSDatabase);
    connect(ui->displayData,&QPushButton::clicked,this,&MainWindow::displayData);
    connect(ui->comboBoxColumns, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::updateSelectedColumn);
    lastFetchedID = -1;
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::connectToAWSDatabase()
{
    ui->connectToDatabase->setEnabled(false);


        ui->statusbar->showMessage("Connecting...........");
        dbAWS = QSqlDatabase::addDatabase("QMYSQL", "ServerConnection");
        dbAWS.setHostName("152.63.32.161");
        dbAWS.setDatabaseName("awstheta_cloud_2023");
        dbAWS.setUserName("root");
        dbAWS.setPassword("");


        if (!dbAWS.open())
        {
            qDebug() << "Error: Failed to connect to  database:" << dbAWS.lastError().text();
            ui->statusbar->showMessage("Failed to connect to Server ");
            ui->connectToDatabase->setEnabled(true);
            return false;
        }

        QSqlQuery columnQuery(dbAWS);
        if (columnQuery.exec("SHOW COLUMNS FROM data")) // Assuming your table name is 'data'
        {
            while (columnQuery.next())
            {
                ColumnNames << columnQuery.value(0).toString();
            }
        }
        else
        {
            qDebug() << "Error fetching column names:" << columnQuery.lastError().text();
            return false;
        }

        customModel->setColumnNames(ColumnNames);
        ui->comboBoxColumns->addItems(ColumnNames);
        // Connect to the local database
        dbLocal = QSqlDatabase::addDatabase("QMYSQL", "LocalConnection");
        dbLocal.setHostName("localhost");
        dbLocal.setDatabaseName("thetadb1");
        dbLocal.setUserName("root");
        dbLocal.setPassword("Theta@2024");



        fetchDataAndInsertToLocalDB();

        qDebug() << "Connected to AWS database!";
        ui->statusbar->showMessage("Connected to AWS database!");
        return true;

}

void MainWindow::fetchDataAndInsertToLocalDB()
{
    if (!dbLocal.open())
    {
        qDebug() << "Error: Failed to connect to local database:" << dbLocal.lastError().text();
        return;
    }

    // Query the AWS database to fetch column names
    QSqlQuery columnQuery(dbAWS);
    if (!columnQuery.exec("SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME = 'data'"))
    {
        qDebug() << "Error retrieving column names:" << columnQuery.lastError().text();
        return;
    }

    QStringList columnNames;
    while (columnQuery.next())
    {
        columnNames << columnQuery.value(0).toString();
    }

    if (columnNames.isEmpty())
    {
        qDebug() << "No columns found in the AWS database.";
        return;
    }

    // Query the AWS database to fetch data
    QSqlQuery query(dbAWS);
    QString queryString = QString("SELECT %1 FROM data WHERE id > %2").arg(columnNames.join(", "), QString::number(lastFetchedID));

    if (query.exec(queryString))
    {
        while (query.next())
        {
            int id = query.value("id").toInt();

            // Check if the ID already exists in the local database
            QSqlQuery checkQuery(dbLocal);
            checkQuery.prepare("SELECT COUNT(*) FROM data WHERE id = ?");
            checkQuery.addBindValue(id);
            if (!checkQuery.exec() || !checkQuery.next())
            {
                qDebug() << "Error checking ID:" << checkQuery.lastError().text();
                continue;  // Skip to the next iteration if there's an error
            }

            int countID = checkQuery.value(0).toInt();
            if (countID > 0)
            {
                // ID already exists in the local database. Skip insertion.
                continue;
            }

            QString timestamp = query.value("datetime").toString();

            // Ensure 'datetime' and 'id' are not included again in the insert query
            QStringList filteredColumns = columnNames;

            filteredColumns.removeAll("id");
            filteredColumns.removeAll("datetime");

            QStringList placeholdersList;
            for (int i = 0; i < filteredColumns.size(); ++i) {
                placeholdersList.append("?");
            }
            QString placeholders = placeholdersList.join(", ");

            QString insertQueryStr = QString("INSERT INTO data (id, datetime, %1) VALUES (?, ?, %2)")
                                         .arg(filteredColumns.join(", "), placeholders);

            QSqlQuery insertQuery(dbLocal);
            insertQuery.prepare(insertQueryStr);
            insertQuery.addBindValue(id);
            insertQuery.addBindValue(timestamp);

            for (const QString &col : filteredColumns)
            {
                insertQuery.addBindValue(query.value(col));
            }

            if (!insertQuery.exec())
            {
                qDebug() << "Error executing insert query:" << insertQuery.lastError().text();
            }
        }
    }
    else
    {
        qDebug() << "Error executing query:" << query.lastError().text();
    }
}


void MainWindow::on_refreshDataButton_clicked()
{
    ui->refreshDataButton->setEnabled(false);

    // Connect to the AWS database
    if (!connectToAWSDatabase())
    {
        qDebug() << "Failed to connect to AWS database. Refresh aborted.";
        QMessageBox::information(this, "database not open", "Refresh aborted.");
        ui->refreshDataButton->setEnabled(true);
        return;
    }

    ui->refreshDataButton->setEnabled(true);
    ui->statusbar->showMessage("Connected to AWS database!");

}

void MainWindow::displayData()
{
    if(!dbAWS.isOpen()){
        QMessageBox::information(this, "database not open", "Failed to display data.");
    }

    QSqlQuery query(dbLocal);
    if (query.exec("SELECT * FROM data"))
    {
        QList<QStringList> dataList;
        while (query.next())
        {
            QStringList rowData;
            for (int i = 0; i < query.record().count(); ++i)
            {
                rowData << query.value(i).toString();
            }
            dataList << rowData;
        }
        customModel->setData(dataList);
        ui->tableView->resizeColumnsToContents();
    }
    else
    {
        qDebug() << "Error 3 executing query:" << query.lastError().text();
    }
}
void MainWindow::on_showAverage_clicked()
{
    QString column = selectedColumn();
    QString date = selectedDate();

    if (column.isEmpty() || date.isEmpty())
    {
        qDebug() << "Error: Please select a column and a date.";
        return;
    }

    QString dateTimeStart = date + "T00:00:00.000";
    QString dateTimeEnd = date + "T23:59:59.999";

    QSqlQuery query(dbLocal);
    QString queryString = QString("SELECT %1 FROM data WHERE datetime >= '%2' AND datetime <= '%3'")
                              .arg(column, dateTimeStart, dateTimeEnd);
    if (query.exec(queryString))
    {
        double sum = 0.0;
        int count = 0;
        while (query.next())
        {
            sum += query.value(0).toDouble();
            count++;
        }
        if (count > 0)
        {
            double average = sum / count;
            ui->averageData->setText(QString::number(average)); // Display the average value
            qDebug() << "Average: " << average;
        }
        else
        {
            qDebug() << "No data found for" << date;
            QMessageBox::information(this, "No Data Found", "No data found for the selected date.");
        }
    }
    else
    {
        qDebug() << "Error 4 executing query:" << query.lastError().text();
    }
}
void MainWindow::updateSelectedColumn(int index)
{
    selectedColumnIndex = index;
}
QString MainWindow::selectedColumn()
{
    if (selectedColumnIndex >= 0 && selectedColumnIndex < ColumnNames.size())
        return ColumnNames[selectedColumnIndex];
    else
        return "";
}

QString MainWindow::selectedDate()
{
    return ui->dateEdit->date().toString("yyyy-MM-dd");
}


void MainWindow::on_showGraph_clicked()
{
    // Get the selected column
    QString selectedColumnName = selectedColumn();
    if (selectedColumnName.isEmpty())
    {
        qDebug() << "Error: Please select a column.";
        QMessageBox::information(this, "Message", "Please select a column.");
        return;
    }


    QVector<double> xData;
    QVector<double> yData;

    // Retrieve data from the model
    for (int row = 0; row < customModel->rowCount(); ++row)
    {
        int columnIndex = customModel->columnIndex(selectedColumnName);
        QModelIndex index = customModel->index(row, columnIndex);
        QVariant data = customModel->data(index);
        qDebug()<<data;
        double value = data.toDouble();
        yData.append(value);
        xData.append(row); // Use row index as x value
    }
    qDebug()<<"Y value :"<<yData;

    ui->customPlot->clearGraphs();
    ui->customPlot->addGraph();
    ui->customPlot->graph(0)->setData(xData, yData);
    ui->customPlot->xAxis->setLabel("Index");
    ui->customPlot->yAxis->setLabel(selectedColumnName);
    ui->customPlot->rescaleAxes();
    ui->customPlot->replot();

    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->indexOf(ui->GraphPage));
}


void MainWindow::on_InsertDataPagebutton_clicked()
{
    if(dbAWS.isOpen())
     ui->stackedWidget->setCurrentIndex(ui->stackedWidget->indexOf(ui->InsertDataPage));

    else
     QMessageBox::information(this, "Failed ", "Server is not connected");

}


void MainWindow::on_pushButtonSend_clicked()
{


    QString inputData = ui->textEditDataSend->toPlainText();
    QStringList inputValues = inputData.split(',');


    QVector<double> values;
    for (const QString &valueStr : inputValues)
    {
        double value = valueStr.trimmed().toDouble();
        values.append(value);
    }

    // Connect to the AWS database
    if (!connectToAWSDatabase())
    {
        qDebug() << "Failed to connect to AWS database. Insertion aborted.";
        return;
    }

    // Construct the insertion query
    QString queryStr = "INSERT INTO data (data";
    QString valuesStr = "VALUES (?";
    for (int i = 2; i <= values.size(); ++i) {
        queryStr += QString(", data%1").arg(i);
        valuesStr += ", ?";
    }
    queryStr += ") " + valuesStr + ")";

    // Insert data into the AWS database
    QSqlQuery insertQuery(dbAWS);
    insertQuery.prepare(queryStr);
    for (double value : values) {
        insertQuery.addBindValue(value);
    }

    if (!insertQuery.exec())
    {
        qDebug() << "Error executing insert query 1:" << insertQuery.lastError().text();
        QMessageBox::critical(this, "Error", "Failed to insert data into AWS database.");
        return;
    }

    qDebug() << "Data inserted into AWS database successfully.";
    QMessageBox::information(this, "Success", "Data inserted into AWS database successfully.");
}


void MainWindow::on_pushButtonHome_clicked()
{
    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->indexOf(ui->Datapage));
}

