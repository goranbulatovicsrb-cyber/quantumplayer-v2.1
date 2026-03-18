#include "smartplaylistdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QDialogButtonBox>

SmartPlaylistDialog::SmartPlaylistDialog(const QList<Track> &allTracks, QWidget *parent)
    : QDialog(parent), m_allTracks(allTracks)
{
    setWindowTitle("Smart Playlist");
    setMinimumWidth(480);
    auto *lay = new QVBoxLayout(this);

    lay->addWidget(new QLabel("<b>Filter tracks by:</b>", this));

    auto *form = new QFormLayout();

    m_fieldBox = new QComboBox(this);
    m_fieldBox->addItems({"Title", "Artist", "Album", "Genre"});
    form->addRow("Field:", m_fieldBox);

    m_opBox = new QComboBox(this);
    m_opBox->addItems({"contains", "starts with", "ends with", "equals"});
    form->addRow("Condition:", m_opBox);

    m_valueEdit = new QLineEdit(this);
    m_valueEdit->setPlaceholderText("Enter value...");
    form->addRow("Value:", m_valueEdit);

    m_limitSpin = new QSpinBox(this);
    m_limitSpin->setRange(0, 9999);
    m_limitSpin->setValue(0);
    m_limitSpin->setSpecialValueText("No limit");
    form->addRow("Limit:", m_limitSpin);

    lay->addLayout(form);

    auto *previewBtn = new QPushButton("Preview", this);
    connect(previewBtn, &QPushButton::clicked, this, &SmartPlaylistDialog::onPreview);
    lay->addWidget(previewBtn);

    auto *previewList = new QListWidget(this);
    previewList->setFixedHeight(180);
    previewList->setObjectName("previewList");
    lay->addWidget(previewList);

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &SmartPlaylistDialog::onAccept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    lay->addWidget(btns);
}

QList<Track> SmartPlaylistDialog::applyFilter() const {
    QString field = m_fieldBox->currentText();
    QString op    = m_opBox->currentText();
    QString val   = m_valueEdit->text().toLower();
    int     limit = m_limitSpin->value();

    QList<Track> result;
    for (const auto &t : m_allTracks) {
        QString src;
        if      (field == "Title")  src = t.title;
        else if (field == "Artist") src = t.artist;
        else if (field == "Album")  src = t.album;
        else if (field == "Genre")  src = t.genre;
        src = src.toLower();

        bool match = false;
        if      (op == "contains")    match = src.contains(val);
        else if (op == "starts with") match = src.startsWith(val);
        else if (op == "ends with")   match = src.endsWith(val);
        else if (op == "equals")      match = (src == val);

        if (match) result.append(t);
        if (limit > 0 && result.size() >= limit) break;
    }
    return result;
}

void SmartPlaylistDialog::onPreview() {
    auto *list = findChild<QListWidget*>("previewList");
    list->clear();
    auto tracks = applyFilter();
    for (const auto &t : tracks)
        list->addItem(QString("%1 — %2").arg(t.title, t.artist));
    if (tracks.isEmpty())
        list->addItem("No tracks match this filter.");
}

void SmartPlaylistDialog::onAccept() {
    m_result = applyFilter();
    accept();
}
