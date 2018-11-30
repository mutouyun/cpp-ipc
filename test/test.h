#pragma once

#include <QtTest>

class TestSuite : public QObject
{
    Q_OBJECT

public:
    explicit TestSuite(void);

protected:
    virtual const char* name(void) const;

protected slots:
    virtual void initTestCase(void);
};
