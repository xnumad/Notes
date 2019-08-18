#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

#include <fstream>
#include <QDebug>
#include <QString>
#include <QMessageBox>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QFileDialog>

using namespace std;

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    //Method    descriptions can be found in the .cpp file
    //Attribute descriptions can be found in this file
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_btnSave_clicked();

    void on_txtNote_textChanged();

    void on_chkAutosave_clicked();

    void on_btnNextNote_clicked();

    void on_btnPreviousNote_clicked();

    void on_btnAdd_clicked();

    void on_btnReload_clicked();

    void on_btnChangeFile_clicked();

private:
    Ui::Widget *ui;
    void loadNotes();
    void readSingleNote();
    bool fileOkay(fstream *file);
    QVector<int> startLines;
    int amountOfLines(std::fstream &file);
    fstream &GotoLine(fstream &file, int num);
    bool setupFile(bool save);
    void validateCurrentLine();

    fstream file; //file object (opened and closed when needed)
    string currentLine = ""; //buffer variable to save the line which is currently being read
    int lineCount = 0; //variable to save the amount of lines the file has
    int currentLineNr = 0; //variable to be able to determine the line number of the current line, increased with every use of getline(), experimental, sadly not integrated in fstream, maybe QFile?
};

#endif // WIDGET_H
