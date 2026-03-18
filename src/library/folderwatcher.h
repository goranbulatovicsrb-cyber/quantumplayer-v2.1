#pragma once
#include <QObject>
#include <QFileSystemWatcher>
#include <QStringList>
#include <QTimer>

class FolderWatcher : public QObject {
    Q_OBJECT
public:
    explicit FolderWatcher(QObject *parent = nullptr);
    void addFolder(const QString &path);
    void removeFolder(const QString &path);
    QStringList watchedFolders() const;

signals:
    void newFilesFound(const QStringList &files);

private slots:
    void onChanged(const QString &path);
    void onScanTimer();

private:
    QFileSystemWatcher *m_watcher;
    QTimer             *m_debounce;
    QStringList         m_pending;
    QStringList         m_known;

    static const QStringList AUDIO_EXTS;
    QStringList scanFolder(const QString &path) const;
};
