#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "widgets/disassemblerview/disassemblerview.h"
#include "dialogs/databasedialog.h"
#include "dialogs/aboutdialog.h"
#include <QDragEnterEvent>
#include <QDesktopWidget>
#include <QMimeDatabase>
#include <QMessageBox>
#include <QMimeData>
#include <QFileDialog>
#include <QFile>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    REDasm::setStatusCallback([this](std::string s) {
        QMetaObject::invokeMethod(this->_lblstatus, "setText", Qt::QueuedConnection, Q_ARG(QString, S_TO_QS(s)));
    });

    REDasm::init(QDir::currentPath().toStdString());

    this->applyTheme();
    ui->setupUi(this);

    this->_lblstatus = new QLabel(this);
    ui->statusBar->addPermanentWidget(this->_lblstatus, 1);

    this->centerWindow();
    this->setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if(!e->mimeData()->hasUrls())
        return;

    e->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *e)
{
    if(!e->mimeData()->hasUrls())
        return;

    e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *e)
{
    const QMimeData* mimedata = e->mimeData();

    if(!mimedata->hasUrls())
        return;

      QList<QUrl> urllist = mimedata->urls();
      QString locfile = urllist.first().toLocalFile();
      QFileInfo fi(locfile);

      if(!fi.isFile())
          return;

      this->load(locfile);
      e->acceptProposedAction();
}

void MainWindow::on_tbOpen_clicked()
{
    QString s = QFileDialog::getOpenFileName(this, "Disassemble file...");

    if(s.isEmpty())
        return;

    this->load(s);
}

void MainWindow::centerWindow()
{
    QRect position = this->frameGeometry();
    position.moveCenter(QDesktopWidget().availableGeometry().center());
    this->move(position.topLeft());
}

void MainWindow::applyTheme()
{
    QFile f(":/themes/application/redasm.css");

    if(!f.open(QFile::ReadOnly))
        return;

    qApp->setStyleSheet(f.readAll());
    f.close();
}

void MainWindow::load(const QString& s)
{
    QFile f(s);

    if(!f.open(QFile::ReadOnly))
        return;

    QFileInfo fi(s);
    QDir::setCurrent(fi.path());
    this->setWindowTitle(fi.fileName());

    this->_loadeddata = f.readAll();
    f.close();

    if(!this->_loadeddata.isEmpty())
        this->initDisassembler();
}

bool MainWindow::checkPlugins(REDasm::FormatPlugin** format, REDasm::ProcessorPlugin** processor)
{
    *format = REDasm::getFormat(reinterpret_cast<u8*>(this->_loadeddata.data()));

    if(!(*format))
    {
        QMessageBox::information(this, "Info", "Unsupported Format");
        return false;
    }

    if(!(*format)->processor())
    {
        QMessageBox::information(this, "Format Error", "Invalid processor");
        return false;
    }

    *processor = REDasm::getProcessor((*format)->processor());

    if(!(*processor))
    {
        QMessageBox::information(this, "Processor not found", QString("Cannot find processor '%1'").arg(QString::fromUtf8((*format)->processor())));
        return false;
    }

    return true;
}

void MainWindow::initDisassembler()
{
    REDasm::Buffer buffer(this->_loadeddata.data(), this->_loadeddata.length());
    DisassemblerView *olddv = NULL, *dv = new DisassemblerView(this->_lblstatus, ui->stackView);
    REDasm::FormatPlugin* format = NULL;
    REDasm::ProcessorPlugin* processor = NULL;

    if(!this->checkPlugins(&format, &processor))
    {
        dv->deleteLater();
        return;
    }

    REDasm::Disassembler* disassembler = new REDasm::Disassembler(buffer, processor, format);
    dv->setDisassembler(disassembler);
    ui->stackView->addWidget(dv);

    QWidget* oldwidget = static_cast<DisassemblerView*>(ui->stackView->widget(0));

    if((olddv = dynamic_cast<DisassemblerView*>(oldwidget)) && olddv->busy())
    {
        connect(olddv, &DisassemblerView::done, [this]() {
            QObject* sender = this->sender();
            ui->stackView->removeWidget(static_cast<QWidget*>(sender));
            sender->deleteLater();
        });

        return;
    }

    ui->stackView->removeWidget(oldwidget);
    oldwidget->deleteLater();
}

void MainWindow::on_tbDatabase_clicked()
{
    DatabaseDialog dlgdatabase(this);
    dlgdatabase.exec();
}

void MainWindow::on_tbAbout_clicked()
{
    AboutDialog dlgabout;
    dlgabout.exec();
}
