#include "widget.h"
#include "ui_widget.h"

using namespace std;

//Repetitive lines in similar methods are not intended to be commented twice. Look for the first occurrence if usage is unclear to find a comment

fstream& Widget::GotoLine(fstream &file, int num) //Method to jump to a specific/absolute/fix line in the file, helpful when indexing the file and to avoid looping going through lines
{
    file.seekg(ios::beg); //Change the current read position to the beginning of the file
    currentLineNr = 1; //update currentLineNr accordingly
    for(int i = 0; i < num - 1; i++){ //instantiate iterator variable i of type 32-bit integer, set loop-condition to be i being smallar than num - 1 and increase i by one after each loop
        file.ignore(numeric_limits<streamsize>::max(),'\n'); //like getline(), but doesn't read the content
    }
    currentLineNr = num;
    return file;
}

int Widget::amountOfLines(fstream& file) //Method to count the amount of lines. Because of its counting method it has the side effect of resetting the read position
{
    int lineCount = 0;
    file.seekg(ios::beg);
    currentLineNr = 0;
    while (!file.eof()) {
        file.ignore(numeric_limits<streamsize>::max(),'\n');
        currentLineNr++;
        lineCount++; //count the lines one by one
    }
    file.seekg(ios::beg);
    currentLineNr = 0;
    return lineCount;
}

bool Widget::setupFile(bool save = false) //Method to open a file stream with requested permissions and return its opening status
{
    if (save) {
        file.open(ui->txtFilePath->text().toStdString(), ios::out|ios::trunc);
    } else {
        file.open(ui->txtFilePath->text().toStdString(), ios::in);
    }
    if (!file.good()) { //if file isn't okay
        ui->lblStatusMessage->setText("File does not include note information");
        return false; //tell the calling method to not even try reading a non-existant file
    }
    return true;
}

void Widget::validateCurrentLine() //Method to validate the string to only contain numbers, doesn't handle negative numbers and non-whole numbers (not needed in this use case anyways)
{
    //previous approach
    //return !currentLine.empty() && find_if(currentLine.begin(),
    //    currentLine.end(), [](char c) { return !isdigit(c); }) == currentLine.end();

    if (currentLine.empty() || currentLine.find_first_not_of("0123456789") != string::npos) { //currentLine is empty or does not only consist of a number
        QString errorMessage = QString(tr("<p>Mismatch in line %1\n"
                                          "<p>Current:\t\"<b>%2</b>\""
                                          "<p>Expected:\tpositive number indicating the length of the following note\n"
                                          )).arg(QString::number(currentLineNr), currentLine.c_str()); //Construct errorMessage with the current line number and its content as placeholder argument
        if (QMessageBox(QMessageBox::Critical, "Error", errorMessage, QMessageBox::Ok | QMessageBox::Ignore).exec() == QMessageBox::Open) { //display errorBox and compare result with "Open button" to check if it was clicked
            QDesktopServices::openUrl(ui->txtFilePath->text()); //open file using default file handler (e.g. text editor)
            exit(1); //Close program (exit with return code 1 (failure)) because of the invalid line
            //qApp->quit(); //shit doesn't work
            //QApplication::quit(); //either
            //QCoreApplication::quit(); //????
        }
        //else it will just continue because the user pressed "Ignore" then
    }
}

void Widget::loadNotes() //Method to load the notes, is being called on startup of the program so that the user can view his previously saved notes
{
    if (!setupFile()) //if setupFile() returned false, meaning it reports the file doesn't exist
        return; //don't load notes

    getline(file, currentLine); //read first line (will count on) of file and save it in currentLine
    currentLineNr++; //respectively update the counter to know in which line we are

    if (file.eof())
        return;
    if (currentLine != versionLine && file.good()) { //if the first line of the file does NOT match versionLine but contains following content
        QString errorMessage = QString(tr("<p>Mismatch in first line\n" //will be rendered as HTML paragraphs //tr() enables translations
                                          "<p>Current:\t\"<b>%1</b>\"" //Tab doesn't seem to properly indent
                                          "<p>Expected:\t\"<b>%2</b>\"\n"
                                          "<p>Open the file in your text editor?")).arg(currentLine.c_str(), versionLine.c_str()); //Construct errorMessage with the current line and versionLine as placeholder argument
        QMessageBox errorBox;
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.setWindowTitle("Warning");
        errorBox.setText("Invalid file");
        errorBox.setInformativeText(errorMessage); //set the previously constructed errorMessage as informative text
        errorBox.setStandardButtons(QMessageBox::Open | QMessageBox::Ignore); //give the user options to ignore the error or open the file
        errorBox.button(QMessageBox::Open)->setText(tr("Open"));
        errorBox.button(QMessageBox::Ignore)->setText(tr("Ignore"));
        errorBox.setDefaultButton(QMessageBox::Open);

        if (errorBox.exec() == QMessageBox::Open) { //display errorBox and compare result with "Open button" to check if it was clicked
            QDesktopServices::openUrl(ui->txtFilePath->text()); //open file using default file handler (e.g. text editor)
            exit(0); //Close program (exit with return code 0 (success)) so the user can focus on the file that has been opened
        }
        //else it will just continue because the user pressed "Ignore" then
    }

    //Build index
    startLines.clear(); //clear index first as it might have already been used by this method, for recalls :)
    lineCount = amountOfLines(file); //Counts line amount and sets position to beginning of file
    getline(file, currentLine); //Skip the first (version) line, go to line 2
    currentLineNr++;
    if (lineCount > 2)
        startLines.append(2); //Save the line in which the first note begins as first entry in the vector
    while (!file.eof()) { //while possible to read the line (indicating the line amount of the current note)
        getline(file, currentLine);
        currentLineNr++;
        validateCurrentLine();
        if (startLines.last() + atoi(currentLine.c_str()) + 1 > lineCount)
            //if the beginning line of the last saved noted
            //+ the value in the current line (to jump)
            //+ 1 (to actually reach the beginning of the next note)
            //are greater than the amount of lines in the file,
            //which occurs when the last note was reached
            //and we want to save the beginning line of the next note
            break;
        startLines.append(startLines.last() + atoi(currentLine.c_str()) + 1); //elsewise append the beginning line of the next note
        GotoLine(file, startLines.last()); //jump to that line so that it can be read on the next loop
        ui->lblNoteSum->setText(QString::number(startLines.size())); //update the UI label "Note 1 of %1" with the currently known amount of note beginnings
    }

    file.close(); //close file stream
}

void Widget::readSingleNote() //Read a single note
{
    if (!setupFile()) //if setupFile() returned false, meaning it reports the file doesn't exist
        return; //don't load the single note

    ui->txtNote->clear();
    GotoLine(file, startLines[ui->lblNoteValue->text().toInt() - 1]); //get the line to jump to by accessing the index vector in which the line of the note is being looked up. the note value itself is being derived directly from the UI
    getline(file, currentLine);
    currentLineNr++;
    validateCurrentLine();

    int linesToRead = atoi(currentLine.c_str()); //will be used to keep the line count of the current note //Convert the number and use it as amount of lines to read
    for (int i = 1; i <= linesToRead; i++) { //for loop to load the note's content, iterates for each line
        getline(file, currentLine);
        currentLineNr++;
        ui->txtNote->appendPlainText(currentLine.c_str()); //repeatedly append currentLine to make the note fully appear
    }

    file.close(); //close file stream
}

void Widget::on_btnNextNote_clicked() //event to load the next note, triggered when the respective button is being clicked
{
    if (ui->lblNoteValue->text().toInt() + 1 >= ui->lblNoteSum->text().toInt()) { //if the current note value would reach the last note
        ui->btnNextNote->setEnabled(false); //disable this button
        ui->lblStatusMessage->setText("(~˘▾˘)~ You can add more notes with the Add button anytime"); //display a hint
        if (ui->lblNoteValue->text().toInt() + 1 > ui->lblNoteSum->text().toInt()) //if it even would be a greater value
            return;
    }
    if (ui->lblNoteValue->text().toInt() + 1 > 1) //if the current note value would reach a note which is not the first
        ui->btnPreviousNote->setEnabled(true); //enable the option to browse back

    loadNotes(); //force index update first to fetch newest file status
    ui->lblNoteValue->setText(QString::number(ui->lblNoteValue->text().toInt() + 1)); //update the current note value
    readSingleNote(); //display the newly chosen note
}

void Widget::on_btnPreviousNote_clicked() //event to load the previous note, triggered when the respective button is being clicked
{
    if (ui->lblNoteValue->text().toInt() - 1 <= 1) { //if the current note value would reach the first note
        ui->btnPreviousNote->setEnabled(false); //disable this button
        if (ui->lblNoteValue->text().toInt() -1 < 1) //if it would be an even lower value
            return;
    }
    if (ui->lblNoteValue->text().toInt() - 1 < ui->lblNoteSum->text().toInt()) //if the current note value would reach a note which is not the last
        ui->btnNextNote->setEnabled(true); //enable the option to browse next

    loadNotes();
    ui->lblNoteValue->setText(QString::number(ui->lblNoteValue->text().toInt() - 1));
    readSingleNote();
}

void Widget::on_btnSave_clicked() //Saving procedure of the file, saveNotes()
{
    QVector<string> notes; //QVector to cache the notes in the file before it will be truncated
    if (setupFile()) //file is good
    {
        //Fill the notes vector
        for (int i = 0; i < startLines.size(); i++) ////note start jumper, iterates for every note
        {
            notes.append(""); //Create a new field/entry in notes vector for the following note
            GotoLine(file, startLines[i]); //Jump to the line in which the note control header starts
            getline(file, currentLine); //Read amount of lines for the following note
            int linesToRead = atoi(currentLine.c_str());
            for (int j = 0; j < linesToRead; j++) ////note content reader, iterates for every line of each note
            {
                getline(file, currentLine);
                if (notes[i] == "") //If note entry is empty yet
                    notes[i] = currentLine; //Set the note entry
                else
                    notes[i] = notes[i] + "\n" + currentLine; //Append the current line to the current note entry with a newline
            }
        }
    }
    else {
        notes.append(""); //Create anyways to save currently open note in it
    }
    file.close();

    //The part where to modify the data.
    //Using the from the file fetched data, it is now being "merged" with the editor data from the UI by overwriting that note
    qDebug() << "Saved: " << ui->txtNote->toPlainText();
    //This line seems to be the culprit for a bug in which if you have two notes of which both only have one line and add a few lines to the first one, the second one looses its content
    notes[ui->lblNoteValue->text().toInt() - 1] = ui->txtNote->toPlainText().toStdString();

    setupFile(true); //open file again, but this time to write (finally actually save) to it
    file << versionLine << endl; //write version line
    for (int i = 0; i < notes.size(); i++) //for each note in the notes vector
    {
        file << count(notes[i].begin(), notes[i].end(), '\n') + 1 << endl; //Write the count of occurrences of \n in the note. Add + 1 to it (because the last line doesn't have that char) and you have the line amount of the note
        file << notes[i].data(); //Write the note itself
        if (i < notes.size() - 1) //on all but the last loop
            file << endl; //prepare an endline for the next note
    }

    file.close();
}

void Widget::on_txtNote_textChanged() //event which is being triggered when the text in the editor field in the UI changed
{
    if (ui->chkAutosave->checkState()) //check if the autosave box is checked
        on_btnSave_clicked(); //if so, run the save procedure
}

void Widget::on_chkAutosave_clicked() //event which is being triggered when the autosave checkbox is being changed/clicked
{
    if (ui->chkAutosave->checkState()) { //check if it was checked
        ui->btnSave->setDisabled(true);
        on_btnSave_clicked(); //withdraw ability to click the save button. It is not needed anymore as every change will be immediately written to the file
    }
    else //it was unchecked
        ui->btnSave->setEnabled(true); //give ability to click the save button again as changes are now only manually being written
}

void Widget::on_btnAdd_clicked() //event which is being triggered when the add button is being clicked
{
    on_btnSave_clicked(); //save file first to be able to subsequently modify it without data loss
    file.open(ui->txtFilePath->text().toStdString(), ios::out|ios::app); //open a file stream in which output is appended to the file
    file << endl << "1" << endl << "Empty note"; //add these two lines resulting in a new note as per the format / notation language
    file.close();
    loadNotes();
    ui->lblNoteValue->setText(ui->lblNoteSum->text()); //set current note value to the highest possible to reach the last note
    readSingleNote(); //read this current note value and display the respective note in the UI
    ui->btnPreviousNote->setEnabled(true);
    ui->btnNextNote->setEnabled(false);
}

void Widget::on_btnReload_clicked() //event which is being triggered when the reload button is being clicked
{
    readSingleNote(); //just call the method to read the currently loaded note again
}

void Widget::on_btnChangeFile_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select file with saved notes"); //returns "" if clicked on "Cancel"
    if (!filePath.isEmpty()) {
        ui->txtFilePath->setText(filePath);

        //Reload notes
        loadNotes();
        readSingleNote();
    }
}

Widget::Widget(QWidget *parent) : //constructor
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    ui->txtFilePath->setText(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/notes_save.txt");
    loadNotes(); //Read file and create index
    readSingleNote(); //Read the first note and display it
}

Widget::~Widget() //destructor
{
    delete ui;
}
