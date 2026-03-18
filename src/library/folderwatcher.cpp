#include "folderwatcher.h"
#include <QDir>
#include <QFileInfo>

const QStringList FolderWatcher::AUDIO_EXTS =
    {"mp3","flac","wav","ogg","m4a","aac","opus","wma"};

FolderWatcher::FolderWatcher(QObject *parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
    , m_debounce(new QTimer(this))
{
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(800);
    connect(m_watcher,  &QFileSystemWatcher::directoryChanged,
            this,        &FolderWatcher::onChanged);
    connect(m_debounce, &QTimer::timeout,
            this,        &FolderWatcher::onScanTimer);
}

void FolderWatcher::addFolder(const QString &path) {
    if (!m_watcher->directories().contains(path)) {
        m_watcher->addPath(path);
        // Initial scan
        QStringList found = scanFolder(path);
        for (const auto &f : found) m_known.append(f);
        if (!found.isEmpty()) emit newFilesFound(found);
    }
}

void FolderWatcher::removeFolder(const QString &path) {
    m_watcher->removePath(path);
}

QStringList FolderWatcher::watchedFolders() const {
    return m_watcher->directories();
}

void FolderWatcher::onChanged(const QString &path) {
    m_pending.append(path);
    m_debounce->start();
}

void FolderWatcher::onScanTimer() {
    QStringList newFiles;
    for (const auto &folder : qAsConst(m_pending)) {
        QStringList files = scanFolder(folder);
        for (const auto &f : files) {
            if (!m_known.contains(f)) {
                m_known.append(f);
                newFiles.append(f);
            }
        }
    }
    m_pending.clear();
    if (!newFiles.isEmpty()) emit newFilesFound(newFiles);
}

QStringList FolderWatcher::scanFolder(const QString &path) const {
    QStringList filters;
    for (const auto &ext : AUDIO_EXTS) filters << "*." + ext;
    QDir dir(path);
    QStringList result;
    for (const auto &fi : dir.entryInfoList(filters, QDir::Files))
        result << fi.absoluteFilePath();
    return result;
}
