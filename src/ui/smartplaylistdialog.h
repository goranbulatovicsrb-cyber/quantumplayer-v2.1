#pragma once
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QList>
#include "../library/track.h"

class SmartPlaylistDialog : public QDialog {
    Q_OBJECT
public:
    explicit SmartPlaylistDialog(const QList<Track> &allTracks,
                                  QWidget *parent = nullptr);
    QList<Track> filteredTracks() const { return m_result; }

private slots:
    void onPreview();
    void onAccept();

private:
    QComboBox  *m_fieldBox;
    QComboBox  *m_opBox;
    QLineEdit  *m_valueEdit;
    QSpinBox   *m_limitSpin;
    QList<Track> m_allTracks;
    QList<Track> m_result;

    QList<Track> applyFilter() const;
};
