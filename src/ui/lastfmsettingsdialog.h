#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>

class LastFmSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit LastFmSettingsDialog(QWidget *parent = nullptr);
    QString apiKey()     const;
    QString apiSecret()  const;
    QString sessionKey() const;
    bool    enabled()    const;
    void    load();
    void    save();
private:
    QLineEdit *m_apiKey;
    QLineEdit *m_apiSecret;
    QLineEdit *m_sessionKey;
    QCheckBox *m_enabled;
};
