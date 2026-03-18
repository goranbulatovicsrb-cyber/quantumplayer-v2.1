#pragma once
#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QScrollArea>

class LyricsWidget : public QWidget {
    Q_OBJECT
public:
    explicit LyricsWidget(QWidget *parent = nullptr);
    void setLyrics(const QString &lyrics, const QString &title, const QString &artist);
    void setLoading(bool loading);
    void clear();
};
