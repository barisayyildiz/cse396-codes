#include "scannedpoints.h"

#include <QtCharts>

ScannedPoints::ScannedPoints(QWidget *parent)
    : QChartView(parent)
{
    chart_ = new QChart();
    this->setChart(chart_);

    // Create a QLineSeries and add it to the chart.
    series_ = new QLineSeries();
    chart_->addSeries(series_);
    chart_->setTitle("Number of scanned points");
    chart_->createDefaultAxes();

    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("Current Step");
    chart_->setAxisX(axisX, series_);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Number of Scanned Points");
    chart_->setAxisY(axisY, series_);

    stepCounter = 0;
    yMax = 0;
}

void ScannedPoints::addNewDataPoint(double y) {
    // Add a new data point to the series.
    series_->append(++stepCounter, y);
    if(y > yMax) {
        yMax = y;
    }
    QAbstractAxis *xAxis = chart_->axisX();
    xAxis->setRange(0, stepCounter + 1);

    QAbstractAxis *yAxis = chart_->axisY();
    yAxis->setRange(0, yMax + 1);
}
