#include "lastfmsettingsdialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSettings>

LastFmSettingsDialog::LastFmSettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Last.fm Scrobbling");
    setMinimumWidth(420);
    auto *lay = new QVBoxLayout(this);

    auto *info = new QLabel(
        "Enter your Last.fm API credentials to enable scrobbling.\n"
        "Get them at: <a href='https://www.last.fm/api/account/create'>"
        "last.fm/api/account/create</a>", this);
    info->setOpenExternalLinks(true);
    info->setWordWrap(true);
    info->setStyleSheet("color:#aaa;font-size:12px;");
    lay->addWidget(info);

    auto *form = new QFormLayout();
    m_apiKey    = new QLineEdit(this);
    m_apiSecret = new QLineEdit(this);
    m_sessionKey= new QLineEdit(this);
    m_sessionKey->setPlaceholderText("Obtain via Last.fm OAuth");
    m_enabled   = new QCheckBox("Enable scrobbling", this);

    form->addRow("API Key:",     m_apiKey);
    form->addRow("API Secret:",  m_apiSecret);
    form->addRow("Session Key:", m_sessionKey);
    form->addRow("",             m_enabled);
    lay->addLayout(form);

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, [this]{ save(); accept(); });
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    lay->addWidget(btns);
    load();
}

QString LastFmSettingsDialog::apiKey()     const { return m_apiKey->text(); }
QString LastFmSettingsDialog::apiSecret()  const { return m_apiSecret->text(); }
QString LastFmSettingsDialog::sessionKey() const { return m_sessionKey->text(); }
bool    LastFmSettingsDialog::enabled()    const { return m_enabled->isChecked(); }

void LastFmSettingsDialog::load() {
    QSettings s("QuantumSoft","QuantumPlayer");
    m_apiKey->setText(s.value("lastfm/apiKey").toString());
    m_apiSecret->setText(s.value("lastfm/apiSecret").toString());
    m_sessionKey->setText(s.value("lastfm/sessionKey").toString());
    m_enabled->setChecked(s.value("lastfm/enabled", false).toBool());
}

void LastFmSettingsDialog::save() {
    QSettings s("QuantumSoft","QuantumPlayer");
    s.setValue("lastfm/apiKey",    m_apiKey->text());
    s.setValue("lastfm/apiSecret", m_apiSecret->text());
    s.setValue("lastfm/sessionKey",m_sessionKey->text());
    s.setValue("lastfm/enabled",   m_enabled->isChecked());
}
