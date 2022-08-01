/*!
    \copyright 2020 Fausto Tomei, Gabriele Antolini,
    Alberto Pistocchi, Marco Bittelli, Antonio Volta, Laura Costantini

    This file is part of AGROLIB.
    AGROLIB has been developed under contract issued by ARPAE Emilia-Romagna

    AGROLIB is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    AGROLIB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with AGROLIB.  If not, see <http://www.gnu.org/licenses/>.

    contacts:
    ftomei@arpae.it
    gantolini@arpae.it
*/

#include "meteo.h"
#include "synchronicityWidget.h"
#include "synchronicityChartView.h"
#include "utilities.h"
#include "interpolation.h"
#include "spatialControl.h"
#include "commonConstants.h"
#include "basicMath.h"
#include "climate.h"
#include "dialogChangeAxis.h"
#include "gammaFunction.h"
#include "furtherMathFunctions.h"
#include "formInfo.h"

#include <QLayout>
#include <QDate>

Crit3DSynchronicityWidget::Crit3DSynchronicityWidget(Crit3DMeteoPointsDbHandler* meteoPointsDbHandler, Crit3DMeteoPoint mp, gis::Crit3DGisSettings gisSettings,
                                                         QDate firstDaily, QDate lastDaily, Crit3DMeteoSettings *meteoSettings, QSettings *settings, Crit3DClimateParameters *climateParameters, Crit3DQuality *quality, Crit3DInterpolationSettings interpolationSettings,
                                                     Crit3DInterpolationSettings qualityInterpolationSettings, bool checkSpatialQuality, Crit3DMeteoPoint *meteoPoints, int nrMeteoPoints)
:meteoPointsDbHandler(meteoPointsDbHandler), mp(mp),firstDaily(firstDaily), gisSettings(gisSettings),
  lastDaily(lastDaily), meteoSettings(meteoSettings), settings(settings), climateParameters(climateParameters),
  quality(quality), interpolationSettings(interpolationSettings), qualityInterpolationSettings(qualityInterpolationSettings), checkSpatialQuality(checkSpatialQuality), meteoPoints(meteoPoints), nrMeteoPoints(nrMeteoPoints)
{
    this->setWindowTitle("Synchronicity analysis Id:"+QString::fromStdString(mp.id));
    this->resize(1240, 700);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setAttribute(Qt::WA_DeleteOnClose);

    // layout
    QVBoxLayout *mainLayout = new QVBoxLayout();
    QHBoxLayout *upperLayout = new QHBoxLayout();
    QHBoxLayout *plotLayout = new QHBoxLayout();

    QGroupBox *firstGroupBox = new QGroupBox();
    QVBoxLayout *firstLayout = new QVBoxLayout();
    QGroupBox *stationGroupBox = new QGroupBox();
    QVBoxLayout *stationLayout = new QVBoxLayout;
    QHBoxLayout *stationRefPointLayout = new QHBoxLayout;
    QHBoxLayout *stationDateLayout = new QHBoxLayout;
    QHBoxLayout *stationButtonLayout = new QHBoxLayout;
    QGroupBox *interpolationGroupBox = new QGroupBox();
    QVBoxLayout *interpolationLayout = new QVBoxLayout;
    QHBoxLayout *interpolationDateLayout = new QHBoxLayout;
    QHBoxLayout *interpolationButtonLayout = new QHBoxLayout;
    referencePointId = "";

    QLabel *nameLabel = new QLabel(QString::fromStdString(mp.name));
    QLabel *variableLabel = new QLabel(tr("Variable: "));
    variable.addItem("DAILY_TMIN");
    variable.addItem("DAILY_TMAX");
    variable.addItem("DAILY_PREC");

    myVar = getKeyMeteoVarMeteoMap(MapDailyMeteoVarToString, variable.currentText().toStdString());
    variable.setSizeAdjustPolicy(QComboBox::AdjustToContents);
    variable.setMaximumWidth(150);
    firstLayout->addWidget(nameLabel);
    firstLayout->addWidget(variableLabel);
    firstLayout->addWidget(&variable);
    firstGroupBox->setLayout(firstLayout);

    stationGroupBox->setTitle("Station");
    QLabel *referenceLabel = new QLabel(tr("Reference point: "));
    stationRefPointLayout->addWidget(referenceLabel);
    stationRefPointLayout->addWidget(&nameRefLabel);
    QLabel *yearFromLabel = new QLabel(tr("From"));
    stationDateLayout->addWidget(yearFromLabel);
    stationYearFrom.setMaximumWidth(100);
    stationDateLayout->addWidget(&stationYearFrom);
    QLabel *yearToLabel = new QLabel(tr("To"));
    stationDateLayout->addWidget(yearToLabel);
    stationYearTo.setMaximumWidth(100);
    stationDateLayout->addWidget(&stationYearTo);
    for(int i = 0; i <= lastDaily.year()-firstDaily.year(); i++)
    {
        stationYearFrom.addItem(QString::number(firstDaily.year()+i));
        stationYearTo.addItem(QString::number(firstDaily.year()+i));
    }
    stationYearTo.setCurrentText(QString::number(lastDaily.year()));
    QLabel *lagLabel = new QLabel(tr("lag"));
    stationDateLayout->addStretch(20);
    stationDateLayout->addWidget(lagLabel);
    stationLag.setRange(-10, 10);
    stationLag.setSingleStep(1);
    stationDateLayout->addWidget(&stationLag);
    stationLayout->addLayout(stationRefPointLayout);
    stationLayout->addLayout(stationDateLayout);

    stationButtonLayout->addWidget(&stationAddGraph);
    stationAddGraph.setText("Add graph");
    stationButtonLayout->addWidget(&stationClearGraph);
    stationClearGraph.setText("Clear");
    stationLayout->addLayout(stationButtonLayout);
    stationGroupBox->setLayout(stationLayout);

    interpolationGroupBox->setTitle("Interpolation");
    QLabel *interpolationYearFromLabel = new QLabel(tr("From"));
    interpolationDateLayout->addWidget(interpolationYearFromLabel);
    interpolationDateFrom.setMaximumWidth(100);
    interpolationDateLayout->addWidget(&interpolationDateFrom);
    QLabel *interpolationYearToLabel = new QLabel(tr("From"));
    interpolationDateLayout->addWidget(interpolationYearToLabel);
    interpolationDateTo.setMaximumWidth(100);
    interpolationDateLayout->addWidget(&interpolationDateTo);
    interpolationDateFrom.setMinimumDate(firstDaily);
    interpolationDateTo.setMinimumDate(firstDaily);
    interpolationDateFrom.setMaximumDate(lastDaily);
    interpolationDateTo.setMaximumDate(lastDaily);
    interpolationDateFrom.setDate(firstDaily);
    interpolationDateTo.setDate(lastDaily);
    interpolationDateLayout->addStretch(20);
    QLabel *interpolationLagLabel = new QLabel(tr("lag"));
    interpolationDateLayout->addWidget(interpolationLagLabel);
    interpolationLag.setRange(-10, 10);
    interpolationLag.setSingleStep(1);
    interpolationDateLayout->addWidget(&interpolationLag);
    QLabel *smoothLabel = new QLabel(tr("smooth"));
    interpolationDateLayout->addWidget(smoothLabel);
    smooth.setRange(0, 10);
    smooth.setSingleStep(1);
    interpolationDateLayout->addWidget(&smooth);
    interpolationLayout->addLayout(interpolationDateLayout);

    QLabel *interpolationElabLabel = new QLabel(tr("Elaboration: "));
    interpolationElab.addItem("Difference");
    interpolationElab.addItem("Absolute difference");
    interpolationElab.addItem("Square difference");
    interpolationButtonLayout->addWidget(interpolationElabLabel);
    interpolationButtonLayout->addWidget(&interpolationElab);
    interpolationButtonLayout->addWidget(&interpolationAddGraph);
    interpolationAddGraph.setText("Add graph");
    interpolationButtonLayout->addWidget(&interpolationClearGraph);
    interpolationClearGraph.setText("Clear");
    interpolationLayout->addLayout(interpolationButtonLayout);
    interpolationGroupBox->setLayout(interpolationLayout);

    synchronicityChartView = new SynchronicityChartView();
    synchronicityChartView->setMinimumWidth(this->width()*2/3);
    plotLayout->addWidget(synchronicityChartView);

    interpolationChartView = new InterpolationChartView();
    interpolationChartView->setMinimumWidth(this->width()*2/3);
    interpolationChartView->setVisible(false);
    plotLayout->addWidget(interpolationChartView);

    upperLayout->addWidget(firstGroupBox);
    upperLayout->addWidget(stationGroupBox);
    upperLayout->addWidget(interpolationGroupBox);

    // menu
    QMenuBar* menuBar = new QMenuBar();
    QMenu *editMenu = new QMenu("Edit");

    menuBar->addMenu(editMenu);
    mainLayout->setMenuBar(menuBar);

    QAction* changeSynchronicityLeftAxis = new QAction(tr("&Change synchronicity chart axis left"), this);
    QAction* changeInterpolationLeftAxis = new QAction(tr("&Change interpolation chart axis left"), this);

    editMenu->addAction(changeSynchronicityLeftAxis);
    editMenu->addAction(changeInterpolationLeftAxis);

    mainLayout->addLayout(upperLayout);
    mainLayout->addLayout(plotLayout);
    setLayout(mainLayout);

    connect(&variable, &QComboBox::currentTextChanged, [=](const QString &newVariable){ this->changeVar(newVariable); });
    connect(&stationYearFrom, &QComboBox::currentTextChanged, [=](){ this->changeYears(); });
    connect(&stationYearTo, &QComboBox::currentTextChanged, [=](){ this->changeYears(); });
    connect(&stationAddGraph, &QPushButton::clicked, [=](){ addStationGraph(); });
    connect(&stationClearGraph, &QPushButton::clicked, [=](){ clearStationGraph(); });
    connect(&interpolationAddGraph, &QPushButton::clicked, [=](){ addInterpolationGraph(); });
    connect(changeSynchronicityLeftAxis, &QAction::triggered, this, &Crit3DSynchronicityWidget::on_actionChangeLeftSynchAxis);
    connect(changeInterpolationLeftAxis, &QAction::triggered, this, &Crit3DSynchronicityWidget::on_actionChangeLeftInterpolationAxis);

    show();
}


Crit3DSynchronicityWidget::~Crit3DSynchronicityWidget()
{

}

void Crit3DSynchronicityWidget::closeEvent(QCloseEvent *event)
{
    emit closeSynchWidget();
    event->accept();
}

void Crit3DSynchronicityWidget::setReferencePointId(const std::string &value)
{
    if (referencePointId != value)
    {
        mpRef.cleanObsDataD();
        mpRef.clear();
        referencePointId = value;
        firstRefDaily = meteoPointsDbHandler->getFirstDate(daily, value).date();
        lastRefDaily = meteoPointsDbHandler->getLastDate(daily, value).date();
        bool hasDailyData = !(firstRefDaily.isNull() || lastRefDaily.isNull());

        if (!hasDailyData)
        {
            QMessageBox::information(nullptr, "Error", "Reference point has not daiy data");
            stationAddGraph.setEnabled(false);
            return;
        }
        QString errorString;
        meteoPointsDbHandler->getPropertiesGivenId(QString::fromStdString(value), &mpRef, gisSettings, errorString);
        nameRefLabel.setText("Id "+ QString::fromStdString(referencePointId)+" - "+QString::fromStdString(mpRef.name));
        stationAddGraph.setEnabled(true);
    }
}


void Crit3DSynchronicityWidget::changeVar(const QString varName)
{
    myVar = getKeyMeteoVarMeteoMap(MapDailyMeteoVarToString, varName.toStdString());
    addStationGraph();
}

void Crit3DSynchronicityWidget::changeYears()
{
    clearStationGraph();
    addStationGraph();
}

void Crit3DSynchronicityWidget::addStationGraph()
{

    if (referencePointId == "")
    {
        QMessageBox::information(nullptr, "Error", "Select a reference point on the map");
        return;
    }
    QDate myStartDate(stationYearFrom.currentText().toInt(), 1, 1);
    QDate myEndDate(stationYearTo.currentText().toInt(), 12, 31);
    int myLag = stationLag.text().toInt();

    std::vector<float> dailyValues;
    QString myError;
    dailyValues = meteoPointsDbHandler->loadDailyVar(&myError, myVar, getCrit3DDate(myStartDate.addDays(myLag)), getCrit3DDate(myEndDate.addDays(myLag)), &firstDaily, &mp);
    if (dailyValues.empty())
    {
        QMessageBox::information(nullptr, "Error", "No data for active station");
        return;
    }
    std::vector<float> dailyRefValues;
    dailyRefValues = meteoPointsDbHandler->loadDailyVar(&myError, myVar, getCrit3DDate(myStartDate), getCrit3DDate(myEndDate), &firstRefDaily, &mpRef);
    if (dailyRefValues.empty())
    {
        QMessageBox::information(nullptr, "Error", "No data for reference station");
        return;
    }

    if (firstDaily.addDays(std::min(0,myLag)) > firstRefDaily)
    {
        if (firstDaily.addDays(std::min(0,myLag)) > QDate(stationYearFrom.currentText().toInt(), 1, 1))
        {
            myStartDate = firstDaily.addDays(std::min(0,myLag));
        }
        else
        {
            myStartDate = QDate(stationYearFrom.currentText().toInt(), 1, 1);
        }
    }
    else
    {
        if (firstRefDaily > QDate(stationYearFrom.currentText().toInt(), 1, 1))
        {
            myStartDate = firstRefDaily;
        }
        else
        {
            myStartDate = QDate(stationYearFrom.currentText().toInt(), 1, 1);
        }

    }

    if (firstDaily.addDays(dailyValues.size()-1-std::max(0,myLag)) < firstRefDaily.addDays(dailyRefValues.size()-1))
    {
        if (firstDaily.addDays(dailyValues.size()-1-std::max(0,myLag)) < QDate(stationYearTo.currentText().toInt(), 12, 31))
        {
            myEndDate = firstDaily.addDays(dailyValues.size()-1-std::max(0,myLag));
        }
        else
        {
            myEndDate = QDate(stationYearTo.currentText().toInt(), 12, 31);
        }

    }
    else
    {
        if (firstRefDaily.addDays(dailyRefValues.size()-1) < QDate(stationYearTo.currentText().toInt(), 12, 31))
        {
            myEndDate = firstRefDaily.addDays(dailyRefValues.size()-1);
        }
        else
        {
            myEndDate =  QDate(stationYearTo.currentText().toInt(), 12, 31);
        }
    }
    QDate currentDate = myStartDate;
    int currentYear = currentDate.year();
    std::vector<float> myX;
    std::vector<float> myY;
    QList<QPointF> pointList;
    float minPerc = meteoSettings->getMinimumPercentage();
    while (currentDate <= myEndDate)
    {
        float myValue1 = dailyValues[firstDaily.daysTo(currentDate)+myLag];
        float myValue2 = dailyRefValues[firstRefDaily.daysTo(currentDate)];
        if (myValue1 != NODATA && myValue2 != NODATA)
        {
            myX.push_back(myValue1);
            myY.push_back(myValue2);
        }
        if ( currentDate == QDate(currentYear, 12, 31)  || currentDate == myEndDate)
        {
            float days = 365;
            if (isLeapYear(currentYear))
            {
                days = 366;
            }
            float r2, y_intercept, trend;

            if ((float)myX.size() / days * 100.0 > minPerc)
            {
                statistics::linearRegression(myX, myY, myX.size(), false, &y_intercept, &trend, &r2);
            }
            else
            {
                r2 = NODATA;
            }
            if (r2 != NODATA)
            {
                pointList.append(QPointF(currentYear,r2));
            }
            myX.clear();
            myY.clear();
            currentYear = currentYear + 1;
        }
        currentDate = currentDate.addDays(1);
    }
    // draw
    synchronicityChartView->drawGraphStation(pointList, variable.currentText(), myLag);

}

void Crit3DSynchronicityWidget::clearStationGraph()
{
    synchronicityChartView->clearStationGraphSeries();
}

void Crit3DSynchronicityWidget::addInterpolationGraph()
{
    QDate myStartDate = interpolationDateFrom.date();
    QDate myEndDate = interpolationDateTo.date();
    int myLag = interpolationLag.text().toInt();
    int mySmooth = smooth.text().toInt();
    QString elabType = interpolationElab.currentText();
    interpolationDailySeries.clear();

    std::vector<float> dailyValues;
    QString myError;
    dailyValues = meteoPointsDbHandler->loadDailyVar(&myError, myVar, getCrit3DDate(myStartDate.addDays(myLag)), getCrit3DDate(myEndDate.addDays(myLag)), &firstDaily, &mp);
    if (dailyValues.empty())
    {
        QMessageBox::information(nullptr, "Error", "No data for active station");
        return;
    }

    int daysToStart = 0;
    int daysToEnd = 0;
    if (firstDaily.addDays(std::min(0,myLag)) > myStartDate)
    {
        daysToStart = myStartDate.daysTo(firstDaily.addDays(std::min(0,myLag)));
        myStartDate = firstDaily.addDays(std::min(0,myLag));
    }
    if (firstDaily.addDays(dailyValues.size()-1-std::max(0,myLag)) < myEndDate)
    {
        daysToEnd = firstDaily.addDays(dailyValues.size()-1-std::max(0,myLag)).daysTo(myEndDate);
        myEndDate = firstDaily.addDays(dailyValues.size()-1-std::max(0,myLag));
    }

    std::vector <Crit3DInterpolationDataPoint> interpolationPoints;

    for (QDate currentDate = myStartDate; currentDate <= myEndDate; currentDate = currentDate.addDays(1))
    {
        float myValue1 = dailyValues[firstDaily.daysTo(currentDate)+myLag];
        // check quality and pass data to interpolation
        if (!checkAndPassDataToInterpolation(quality, myVar, meteoPoints, nrMeteoPoints, getCrit3DTime(currentDate, 1),
                                             &qualityInterpolationSettings, &interpolationSettings, meteoSettings, climateParameters, interpolationPoints,
                                             checkSpatialQuality))
        {
            QMessageBox::critical(nullptr, "Error", "No data available");
            return;
        }

        if (! preInterpolation(interpolationPoints, &interpolationSettings, meteoSettings, climateParameters, meteoPoints, nrMeteoPoints, myVar, getCrit3DTime(currentDate, 1)))
        {
            QMessageBox::critical(nullptr, "Error", "Interpolation: error in function preInterpolation");
            return;
        }
        float interpolatedValue = interpolate(interpolationPoints, &interpolationSettings, meteoSettings, myVar,
                                              float(mp.point.utm.x),
                                              float(mp.point.utm.y),
                                              float(mp.point.z),
                                              mp.getProxyValues(), false);

        if (myValue1 != NODATA && interpolatedValue != NODATA)
        {
            if (elabType == "Difference" || elabType == "Square difference")
            {
                interpolationDailySeries.push_back(myValue1 - interpolatedValue);
            }
            else if (elabType == "Absolute difference")
            {
                interpolationDailySeries.push_back(abs(myValue1 - interpolatedValue));
            }
        }
        else
        {
            interpolationDailySeries.push_back(NODATA);
        }

    }

    // smooth
    smoothSerie();
    // draw
    interpolationChartView->drawGraphInterpolation(smoothInterpDailySeries, myStartDate, variable.currentText(), myLag, mySmooth);

}

void Crit3DSynchronicityWidget::smoothSerie()
{
    int mySmooth = smooth.text().toInt();
    QString elabType = interpolationElab.currentText();
    smoothInterpDailySeries.clear();

    if (mySmooth > 0)
    {
        for (int i = 0; i<interpolationDailySeries.size(); i++)
        {
            float dSum = 0;
            float dSum2 = 0;
            int nDays = 0;
            for (int j = i-mySmooth; j<=i+mySmooth; j++)
            {
                if (j >= 0 && j < interpolationDailySeries.size())
                {
                    if (interpolationDailySeries[j] != NODATA)
                    {
                        dSum = dSum + interpolationDailySeries[j];
                        dSum2 = dSum2 + interpolationDailySeries[j] * interpolationDailySeries[j];
                        nDays = nDays + 1;
                    }
                }
            }
            if (nDays / (mySmooth * 2 + 1) > meteoSettings->getMinimumPercentage() / 100)
            {
                if (elabType == "Square difference")
                {
                    smoothInterpDailySeries.push_back(dSum2/nDays);
                }
                else
                {
                    smoothInterpDailySeries.push_back(dSum/nDays);
                }
            }
            else
            {
                smoothInterpDailySeries.push_back(NODATA);
            }
        }
    }
}

void Crit3DSynchronicityWidget::on_actionChangeLeftSynchAxis()
{
    DialogChangeAxis changeAxisDialog(true);
    if (changeAxisDialog.result() == QDialog::Accepted)
    {
        synchronicityChartView->setYmax(changeAxisDialog.getMaxVal());
        synchronicityChartView->setYmin(changeAxisDialog.getMinVal());
    }
}

void Crit3DSynchronicityWidget::on_actionChangeLeftInterpolationAxis()
{
    DialogChangeAxis changeAxisDialog(true);
    if (changeAxisDialog.result() == QDialog::Accepted)
    {
        interpolationChartView->setYmax(changeAxisDialog.getMaxVal());
        interpolationChartView->setYmin(changeAxisDialog.getMinVal());
    }
}


