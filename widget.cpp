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

        //Disable navigation buttons as there are no notes
        ui->btnNextNote->setEnabled(false);
        ui->btnPreviousNote->setEnabled(false);

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
        invalidFileErrorBox("positive number indicating the length of the following note");
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
        invalidFileErrorBox(versionLine.c_str());
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

    ui->btnPreviousNote->setDisabled(ui->lblNoteValue->text().toInt() == 1); //if reading first note, disable Previous note button, elsewise enable
    ui->btnNextNote->setDisabled(ui->lblNoteValue->text().toInt() == ui->lblNoteSum->text().toInt()); //if reading last note, disable Next note button, elsewise enable

    bool autosave_was_enabled = ui->chkAutosave->isChecked();
    ui->chkAutosave->setChecked(false); //Disable autosave or the automatic saving process would be triggered while changing the textedit in the reading procedure. Gets re-enabled at the end.

    ui->txtNote->clear();
    GotoLine(file, startLines[ui->lblNoteValue->text().toInt() - 1]); //get the line to jump to by accessing the index vector in which the line of the note is being looked up. the note value itself is being derived directly from the UI
    getline(file, currentLine);
    currentLineNr++;
    validateCurrentLine();

    int linesToRead = atoi(currentLine.c_str()); //will be used to keep the line count of the current note //Convert the number and use it as amount of lines to read
    for (int i = 1; i <= linesToRead; i++) { //for loop to load the note's content, iterates for each line
        if (file.eof()) //Unexpected file ending due to higher than actual amount of lines specified for a note
        {
            invalidFileErrorBox("Still note content", "Non-existant line");
            break; //elsewise resulting in the application duplicating lines when reading beyond end of file
            //The application will cut the note (decrease the specified amount of lines) to the actual amount of lines when saving.
        }
        getline(file, currentLine);
        currentLineNr++;
        ui->txtNote->appendPlainText(currentLine.c_str()); //repeatedly append currentLine to make the note fully appear
    }

    file.close(); //close file stream

    ui->chkAutosave->setChecked(autosave_was_enabled); //Now that the text in the textedit is not being changed by the reading procedure anymore, which would have triggered autosave, re-enable autosave if it was enabled
}

void Widget::on_btnNextNote_clicked() //event to load the next note, triggered when the respective button is being clicked
{
    loadNotes(); //force index update first to fetch newest file status
    ui->lblNoteValue->setText(QString::number(ui->lblNoteValue->text().toInt() + 1)); //update the current note value
    readSingleNote(); //display the newly chosen note
}

void Widget::on_btnPreviousNote_clicked() //event to load the previous note, triggered when the respective button is being clicked
{
    loadNotes();
    ui->lblNoteValue->setText(QString::number(ui->lblNoteValue->text().toInt() - 1));
    readSingleNote();
}

void Widget::on_btnSave_clicked() //Saving procedure of the file, saveNotes()
{
    QVector<string> notes; //QVector to cache the notes in the file before it will be truncated
    loadNotes(); //Rebuild index to prevent the save procedure from reading an empty file (because file.good() returns true and setupFile() will also succeed) that was only created during the attempt to read a non-existant notes file.

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
    bool autosave = ui->chkAutosave->checkState();
    ui->btnSave->setDisabled(autosave); //If autosave is enabled, disable Save button as every change will be written to the file immediately makes saving a redundant option
    ui->btnDiscard->setDisabled(autosave); //Also disable Discard button because the changes can't be discarded when they're always being saved directly
    on_btnSave_clicked(); //Save note in its current state because autosave is already on. If it is being disabled, this operation is unneccessary but doesn't change anything because the note is already saved in its current state by autosave anyways.
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
    ui->txtNote->selectAll();
    ui->txtNote->setFocus();
    ui->btnPreviousNote->setEnabled(true);
    ui->btnNextNote->setEnabled(false);
}

void Widget::on_btnDiscard_clicked() //event which is being triggered when the reload button is being clicked
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

void Widget::invalidFileErrorBox(QString expectedLineContent, QString currentLineContent)
{
    if (currentLineContent == nullptr) //default value
        currentLineContent = currentLine.c_str();

    QMessageBox errorBox;
    errorBox.setWindowTitle("Error");
    errorBox.setIcon(QMessageBox::Critical);
    errorBox.setText("Invalid file");
    errorBox.setInformativeText(QString("<p>Mismatch in line %1\n"
                                        "<p>Current:\t<b>%2</b>"
                                        "<p>Expected:\t<b>%3</b>\n"
                                        "<p>Open the file in your text editor?"
                                        ).arg(QString::number(currentLineNr), currentLineContent, expectedLineContent));
    errorBox.setStandardButtons(QMessageBox::Open | QMessageBox::Ignore);

    if (errorBox.exec() == QMessageBox::Open) { //display errorBox and compare result with "Open button" to check if it was clicked
        QDesktopServices::openUrl(ui->txtFilePath->text()); //open file using default file handler (e.g. text editor)
        exit(0); //Intentionally (hence with exit code 0 (success)) close program for the user to inspect the file
    }
    //else it will just continue because the user pressed "Ignore" then
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
