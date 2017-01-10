#ifndef MEMBERWELCOME_P_H
#define MEMBERWELCOME_P_H

#include "sqldata_p.h"
#include "memberwelcome.h"

class MemberWelcomePrivate : public SqlDataPrivate
{
    Q_DECLARE_PUBLIC(MemberWelcome)

public:
    MemberWelcomePrivate();
    virtual ~MemberWelcomePrivate();
};

#endif // MEMBERWELCOME_P_H
