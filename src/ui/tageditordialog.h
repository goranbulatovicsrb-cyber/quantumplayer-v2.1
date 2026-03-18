#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include "../library/track.h"

class TagEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit TagEditorDialog(const Track &track, QWidget *parent = nullptr);
    Track updatedTrack() const;

private:
    Track      m_track;
    QLineEdit *m_title;
    QLineEdit *m_artist;
    QLineEdit *m_album;
    QLineEdit *m_genre;
    QSpinBox  *m_year;
};
