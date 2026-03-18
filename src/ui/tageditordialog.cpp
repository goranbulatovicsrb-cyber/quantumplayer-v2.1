#include "tageditordialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>

TagEditorDialog::TagEditorDialog(const Track &track, QWidget *parent)
    : QDialog(parent), m_track(track)
{
    setWindowTitle("Edit Track Info");
    setMinimumWidth(400);
    auto *lay = new QVBoxLayout(this);

    // File path info
    auto *pathLbl = new QLabel(track.fileUrl.fileName(), this);
    pathLbl->setStyleSheet("color:#888;font-size:11px;");
    pathLbl->setWordWrap(true);
    lay->addWidget(pathLbl);

    auto *form = new QFormLayout();

    m_title  = new QLineEdit(track.title, this);
    m_artist = new QLineEdit(track.artist, this);
    m_album  = new QLineEdit(track.album, this);
    m_genre  = new QLineEdit(track.genre, this);
    m_year   = new QSpinBox(this);
    m_year->setRange(0, 2100);
    m_year->setValue(track.year > 0 ? track.year : 2024);
    m_year->setSpecialValueText("Unknown");

    form->addRow("Title:",  m_title);
    form->addRow("Artist:", m_artist);
    form->addRow("Album:",  m_album);
    form->addRow("Genre:",  m_genre);
    form->addRow("Year:",   m_year);
    lay->addLayout(form);

    auto *note = new QLabel("Note: Changes are saved in Quantum Player only,\nnot written back to the audio file.", this);
    note->setStyleSheet("color:#666;font-size:11px;font-style:italic;");
    lay->addWidget(note);

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    lay->addWidget(btns);
}

Track TagEditorDialog::updatedTrack() const {
    Track t = m_track;
    t.title  = m_title->text();
    t.artist = m_artist->text();
    t.album  = m_album->text();
    t.genre  = m_genre->text();
    t.year   = m_year->value();
    return t;
}
