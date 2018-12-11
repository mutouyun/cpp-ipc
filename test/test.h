#pragma once

#include <QtTest>

class TestSuite : public QObject
{
    Q_OBJECT

public:
    explicit TestSuite();

protected:
    virtual const char* name() const;

protected slots:
    virtual void initTestCase();
};
