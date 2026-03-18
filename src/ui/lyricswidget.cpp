#include "lyricswidget.h"
#include <QVBoxLayout>

LyricsWidget::LyricsWidget(QWidget *parent) : QWidget(parent) {
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(12,8,12,8);
}

void LyricsWidget::setLyrics(const QString &lyrics, const QString &title, const QString &artist) {
    // Clear and rebuild
    QLayoutItem *item;
    while ((item = layout()->takeAt(0))) {
        delete item->widget();
        delete item;
    }
    auto *titleLbl = new QLabel(QString("<b>%1</b> — %2").arg(title, artist), this);
    titleLbl->setStyleSheet("font-size:13px; color:#aaa; margin-bottom:8px;");
    titleLbl->setWordWrap(true);
    layout()->addWidget(titleLbl);

    auto *txt = new QTextEdit(this);
    txt->setReadOnly(true);
    txt->setPlainText(lyrics);
    txt->setStyleSheet("background:transparent;border:none;color:#ddd;font-size:13px;line-height:1.6;");
    layout()->addWidget(txt);
}

void LyricsWidget::setLoading(bool loading) {
    QLayoutItem *item;
    while ((item = layout()->takeAt(0))) { delete item->widget(); delete item; }
    auto *lbl = new QLabel(loading ? "🔍  Fetching lyrics..." : "No lyrics found.", this);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setStyleSheet("color:#666;font-size:13px;");
    layout()->addWidget(lbl);
}

void LyricsWidget::clear() { setLoading(false); }
