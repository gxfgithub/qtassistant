#include "cqassistant.h"
#include "cqassistant_p.h"

#include <QCoreApplication>
#include <QStringBuilder>
#include <QTextStream>

#include "datas/masterlevels.h"
#include "datas/memberdeathhouse.h"

bool cqStartsWith(const char *str, const char *pre)
{
    return (strncmp(str, pre, strlen(pre)) == 0);
}

// class CqAssistant

bool CqAssistant::privateMessageEventFilter(const MessageEvent &ev)
{
    Q_UNUSED(ev);
    return false;
}

bool CqAssistant::groupMessageEventFilter(const MessageEvent &ev)
{
    Q_D(CqAssistant);
    QStringList args;

    if ((ev.gbkMsg[0] == '!')) {
        QString msg = conv(ev.gbkMsg).mid(1);
        args = msg.split(' ', QString::SkipEmptyParts);
    }
    if (cqStartsWith(ev.gbkMsg, "sudo")) {
        QString msg = conv(ev.gbkMsg).mid(4);
        args = msg.split(' ', QString::SkipEmptyParts);
    }

    if (!args.isEmpty()) {
        QString c = args.value(0);

        if (c == QLatin1String("h") || c == QLatin1String("help")) {
            d->groupHelp(ev, args.mid(1));
            return true;
        }
        if (c == QLatin1String("l") || c == QLatin1String("level")) {
            d->groupLevel(ev, args.mid(1));
            return true;
        }
        if (c == QLatin1String("r") || c == QLatin1String("rename")) {
            d->groupRename(ev, args.mid(1));
            return true;
        }

        if (c == QLatin1String("b") || c == QLatin1String("ban")) {
            d->groupBan(ev, args.mid(1));
            return true;
        }
        if (c == QLatin1String("k") || c == QLatin1String("kill")) {
            d->groupKill(ev, args.mid(1));
            return true;
        }
        if (c == QLatin1String("p") || c == QLatin1String("power")) {
            d->groupPower(ev, args.mid(1));
            return true;
        }

        if (c == QLatin1String("ub") || c == QLatin1String("unban")) {
            d->groupUnban(ev, args.mid(1));
            return true;
        }
        if (c == QLatin1String("uk") || c == QLatin1String("unkill")) {
            d->groupUnkill(ev, args.mid(1));
            return true;
        }
        if (c == QLatin1String("up") || c == QLatin1String("unpower")) {
            d->groupUnpower(ev, args.mid(1));
            return true;
        }
    }

    return false;
}

bool CqAssistant::discussMessageEventFilter(const MessageEvent &ev)
{
    Q_UNUSED(ev);
    return false;
}

// class CqAssistantPrivate

void CqAssistantPrivate::groupHelp(const MessageEvent &ev, const QStringList &args)
{
    Q_Q(CqAssistant);

    Q_UNUSED(args);

    QString help;

    help += QString::fromUtf8("命令清单: \n");
    help += QString::fromUtf8("--帮助信息: h,help\n");
    help += QString::fromUtf8("--查询等级: l,level\n");
    help += QString::fromUtf8("--修改昵称: r,rename\n");
    help += QString::fromUtf8("--禁言命令: b,ban\n");
    help += QString::fromUtf8("--取消禁言: ub,unban\n");
    help += QString::fromUtf8("--踢出命令: k,kill\n");
    help += QString::fromUtf8("--取消踢出: uk,unkill\n");
    help += QString::fromUtf8("--提权命令: p,power\n");
    help += QString::fromUtf8("--取消提权: up,unpower\n");

    q->sendGroupMessage(ev.from, help);
}

void CqAssistantPrivate::groupLevel(const MessageEvent &ev, const QStringList &args)
{
    Q_Q(CqAssistant);

    bool argvGlobal = false;
    bool argvList = false;

    LevelInfoList ll;
    bool invalidArgs = false;
    for (const QString &arg : args) {
        if (arg.startsWith("[CQ:at")) {
            int i = arg.lastIndexOf("]");
            QString uid = arg.mid(10, i - 10);
            ll.append(LevelInfo(uid.toLongLong(),
                                MasterLevel::Unknown));
        } else if (arg == QLatin1String("g")) {
            argvGlobal = true;
        } else if (arg == QLatin1String("global")) {
            argvGlobal = true;
        } else if (arg == QLatin1String("l")) {
            argvList = true;
        } else if (arg == QLatin1String("list")) {
            argvList = true;
        } else {
            invalidArgs = true;
            break;
        }
    }

    if (!invalidArgs) {
        if (ll.isEmpty()) {
            if (argvList) {
                QStringList members;
                members.append(tr("Level List:"));
                ll = levels->levels(argvGlobal ? 0 : ev.from);

                for (const LevelInfo &li : ll) {
                    QString name = q->msgAt(li.uid);
                    QString level = MasterLevels::levelName(li.level);
                    members.append(tr("%1: %2").arg(level, name));
                }

                q->sendGroupMessage(ev.from, members.join("\n"));
            } else {
                MasterLevel level = levels->level(argvGlobal ? 0 : ev.from, ev.sender);
                QString reply = tr("Level: %1 %2").arg(MasterLevels::levelName(level), q->msgAt(ev.sender));
                q->sendGroupMessage(ev.from, reply);
            }

            return;
        } else if (false == argvList) {

            QStringList members;
            members.append(tr("Level List:"));
            levels->update(argvGlobal ? 0 : ev.from, ll);

            for (const LevelInfo &li : ll) {
                QString name = q->msgAt(li.uid);
                QString level = MasterLevels::levelName(li.level);
                members.append(tr("%1: %2").arg(level, name));
            }

            q->sendGroupMessage(ev.from, members.join("\n"));

            return;
        }
    }

    q->sendGroupMessage(ev.from, "usage");
}

void CqAssistantPrivate::groupRename(const MessageEvent &ev, const QStringList &args)
{
    Q_Q(CqAssistant);

    MasterLevel level = levels->level(ev.from, ev.sender);
    if (level > MasterLevel::Master5) {
        QString name = q->msgAt(ev.sender);
        QString levelName = MasterLevels::levelName(level);
        QString reply = tr("Permission Denied: %1 %2").arg(levelName, name);
        q->sendGroupMessage(ev.from, reply);
        return;
    }

    QString name = q->conv(ev.gbkMsg);

    LevelInfoList ll;
    bool prefixFound = false;
    bool invalidArgs = false;
    for (const QString &arg : args) {
        if (arg.startsWith("[CQ:at")) {
            int i = arg.lastIndexOf("]");
            QString uid = arg.mid(10, i - 10);
            ll.append(LevelInfo(uid.toLongLong(),
                                MasterLevel::Unknown));

            if (prefixFound) {
                invalidArgs = true;
                break;
            }

            if (ll.count() == 1) {
                int r = name.indexOf(arg);
                name = name.mid(r + arg.count());
            } else {
                invalidArgs = true;
                break;
            }
        } else if (ll.isEmpty()) {
            prefixFound = true;
            int r = name.indexOf(arg);
            name = name.mid(r);
        }
    }

    //newName = newName.replace("[", "[");
    //newName = newName.replace("]", "]");

    name = name.trimmed();

    if (!invalidArgs && !name.isEmpty()) {
        qint64 uid = 0;
        if (ll.isEmpty()) {
            uid = ev.sender;
        } else {
            uid = ll.at(0).uid;
        }

        q->renameGroupMember(ev.from, uid, name);

        QString reply = tr("Nickname Changed: %1").arg(q->msgAt(uid));
        q->sendGroupMessage(ev.from, reply);

        return;
    }

    do {
        QString usage;
        QTextStream ts(&usage);

        ts << QString::fromUtf8("用法: \n");
        ts << QString::fromUtf8("  sudo rename @成员 新昵称\n");
        ts << QString::fromUtf8("举例:\n");
        ts << QString::fromUtf8("  sudo rename @[北京]猫大 [北京]大猫\n");

        ts.flush();

        q->sendGroupMessage(ev.from, usage);
    } while (false);
}

void CqAssistantPrivate::groupBan(const MessageEvent &ev, const QStringList &args)
{
    Q_Q(CqAssistant);

    MasterLevel level = levels->level(ev.from, ev.sender);
    if (level > MasterLevel::Master5) {
        QString name = q->msgAt(ev.sender);
        QString levelName = MasterLevels::levelName(level);
        QString reply = tr("Permission Denied: %1 %2").arg(levelName, name);
        q->sendGroupMessage(ev.from, reply);
        return;
    }

    int ds = 0;
    int hs = 0;
    int ms = 0;

    LevelInfoList ll;
    bool invalidArgs = false;
    for (const QString &arg : args) {
        if (arg.startsWith("[CQ:at")) {
            int i = arg.lastIndexOf("]");
            QString str = arg.mid(10, i - 10);
            ll.append(LevelInfo(str.toLongLong(),
                                MasterLevel::Unknown));
        } else if (arg.endsWith("d")) {
            ds = arg.left(arg.length() - 1).toInt();
        } else if (arg.endsWith("h")) {
            hs = arg.left(arg.length() - 1).toInt();
        } else if (arg.endsWith("m")) {
            ms = arg.left(arg.length() - 1).toInt();
        } else if (arg.startsWith("d")) {
            ds = arg.mid(1).toInt();
        } else if (arg.startsWith("h")) {
            hs = arg.mid(1).toInt();
        } else if (arg.startsWith("m")) {
            ms = arg.mid(1).toInt();
        } else {
            invalidArgs = true;
            break;
        }
    }

    if (!invalidArgs && !ll.isEmpty()) {
        int duration = ds * 86400 + hs * 3600 + ms * 60;
        if (duration > 86400 * 30) {
            duration = 86400 * 30;
        } else if (duration <= 60) {
            duration = 60;
        }

        QStringList masters;
        QStringList members;

        levels->update(ev.from, ll);
        for (const LevelInfo &li : ll) {
            if (li.level <= level) {
                masters << q->msgAt(li.uid);
            } else {
                q->banGroupMember(ev.from, li.uid, duration);
                members << q->msgAt(li.uid);
            }
        }

        QStringList reply;
        if (!masters.isEmpty()) {
            reply << tr("Permission Denied:");
            reply << masters;
        }
        if (!members.isEmpty()) {
            reply << tr("Ban List:");
            reply << members;
        }
        q->sendGroupMessage(ev.from, reply.join("\n"));

        return;
    }

    do {
        QString usage;
        QTextStream ts(&usage);

        ts << QString::fromUtf8("用法: \n");
        ts << QString::fromUtf8("  sudo ban [天数d] [时数h] [分数m] @成员1 [@成员2] [@成员3] ...\n");
        ts << QString::fromUtf8("  可同时多人禁言，最长时间不能超过 30 天，最短时间不能 1 分钟。\n");
        ts << QString::fromUtf8("举例:\n");
        ts << QString::fromUtf8("  将 @[北京]猫大 禁言两天\n");
        ts << QString::fromUtf8("  sudo ban @[北京]猫大 2d\n");
        ts << QString::fromUtf8("  将 @[北京]猫大 @[广州]猫咪 禁言一天六小时三十分钟\n");
        ts << QString::fromUtf8("  sudo ban 1d 6h 30m @[北京]猫大 @[广州]猫咪\n");

        ts.flush();

        q->sendGroupMessage(ev.from, usage);
    } while (false);
}

void CqAssistantPrivate::groupKill(const MessageEvent &ev, const QStringList &args)
{
    Q_Q(CqAssistant);

    MasterLevel level = levels->level(ev.from, ev.sender);
    if (level > MasterLevel::Master3) {
        QString name = q->msgAt(ev.sender);
        QString levelName = MasterLevels::levelName(level);
        QString reply = tr("Permission Denied: %1 %2").arg(levelName, name);
        q->sendGroupMessage(ev.from, reply);
        return;
    }

    LevelInfoList ll;
    bool invalidArgs = false;
    for (const QString &arg : args) {
        if (arg.startsWith("[CQ:at")) {
            int i = arg.lastIndexOf("]");
            QString str = arg.mid(10, i - 10);
            ll.append(LevelInfo(str.toLongLong(),
                                MasterLevel::Unknown));
        } else {
            invalidArgs = true;
            break;
        }
    }

    if (!invalidArgs && !ll.isEmpty()) {
        QStringList masters;
        QStringList members;

        levels->update(ev.from, ll);
        for (const LevelInfo &li : ll) {
            if (li.level <= MasterLevel::Master5) {
                masters << q->msgAt(li.uid);
            } else {
                deathHouse->killMember(ev.from, li.uid);
                members << q->msgAt(li.uid);
            }
        }

        QStringList reply;
        if (!masters.isEmpty()) {
            reply << tr("Permission Denied:");
            reply << masters;
        }
        if (!members.isEmpty()) {
            reply << tr("Kill List:");
            reply << members;
        }
        q->sendGroupMessage(ev.from, reply.join("\n"));

        return;
    }

    q->sendGroupMessage(ev.from, tr("kill @Member1 [@Member2] [@Member3] ..."));
}

void CqAssistantPrivate::groupPower(const MessageEvent &ev, const QStringList &args)
{
    Q_Q(CqAssistant);

    MasterLevel level = levels->level(ev.from, ev.sender);
    if (level > MasterLevel::Master1) {
        QString name = q->msgAt(ev.sender);
        QString levelName = MasterLevels::levelName(level);
        QString reply = tr("Permission Denied: %1 %2").arg(levelName, name);
        q->sendGroupMessage(ev.from, reply);
        return;
    }

    int levelValue = 0;
    bool argvGlobal = false;

    LevelInfoList ll;
    bool invalidArgs = false;
    for (const QString &arg : args) {
        if (arg.startsWith("[CQ:at")) {
            int i = arg.lastIndexOf("]");
            QString str = arg.mid(10, i - 10);
            ll.append(LevelInfo(str.toLongLong(),
                                MasterLevel::Unknown));
        } else if (arg.startsWith("m")) {
            levelValue = arg.mid(1).toInt();
        } else if (arg == QLatin1String("g")) {
            argvGlobal = true;
        } else if (arg == QLatin1String("global")) {
            argvGlobal = true;
        } else {
            invalidArgs = true;
            break;
        }
    }

    do {
        if (!invalidArgs && !ll.isEmpty()) {
            MasterLevel newLevel = MasterLevel::Unknown;
            switch (levelValue) {
            case 1:
                newLevel = MasterLevel::Master1;
                break;
            case 2:
                newLevel = MasterLevel::Master2;
                break;
            case 3:
                newLevel = MasterLevel::Master3;
                break;
            case 4:
                newLevel = MasterLevel::Master4;
                break;
            case 5:
                newLevel = MasterLevel::Master5;
                break;
            }
            if (MasterLevel::Unknown == newLevel) {
                break;
            }

            if (newLevel <= level) {
                QString name = q->msgAt(ev.sender);
                QString levelName = MasterLevels::levelName(level);
                QString reply = tr("Permission Denied: %1 %2").arg(levelName, name);
                q->sendGroupMessage(ev.from, reply);
                return;
            }

            if (argvGlobal) {
                if (level != MasterLevel::ATField) {
                    QString name = q->msgAt(ev.sender);
                    QString levelName = MasterLevels::levelName(level);
                    QString reply = tr("Permission Denied: %1 %2").arg(levelName, name);
                    q->sendGroupMessage(ev.from, reply);
                    return;
                }
            }

            QStringList impowers;
            QStringList unchanges;
            QStringList depowers;

            levels->update(ev.from, ll);
            for (const LevelInfo &li : ll) {
                if (li.level <= level) {
                    QString name = q->msgAt(ev.sender);
                    QString levelName = MasterLevels::levelName(level);
                    QString reply = tr("Permission Denied: %1 %2").arg(levelName, name);
                    q->sendGroupMessage(ev.from, reply);
                    return;
                }
            }

            for (const LevelInfo &li : ll) {
                levels->setLevel(argvGlobal ? 0 : ev.from, li.uid, newLevel);

                if (newLevel < li.level) {
                    impowers << tr("%1 > %2: %3").arg(MasterLevels::levelName(li.level), MasterLevels::levelName(newLevel), q->msgAt(li.uid));
                } else if (newLevel == li.level) {
                    unchanges << tr("%1: %3").arg(MasterLevels::levelName(li.level), q->msgAt(li.uid));
                } else {
                    depowers << tr("%1 > %2: %3").arg(MasterLevels::levelName(li.level), MasterLevels::levelName(newLevel), q->msgAt(li.uid));
                }
            }

            QStringList reply;
            if (!impowers.isEmpty()) {
                if (argvGlobal) {
                    reply << tr("Impower List(Global):");
                } else {
                    reply << tr("Impower List:");
                }
                reply << impowers;
            }
            if (!unchanges.isEmpty()) {
                if (argvGlobal) {
                    reply << tr("Unchange List(Global):");
                } else {
                    reply << tr("Unchange List:");
                }
                reply << unchanges;
            }
            if (!depowers.isEmpty()) {
                if (argvGlobal) {
                    reply << tr("Depower List(Global):");
                } else {
                    reply << tr("Depower List:");
                }
                reply << depowers;
            }
            q->sendGroupMessage(ev.from, reply.join("\n"));

            return;
        }
    } while (false);

    q->sendGroupMessage(ev.from, tr("power m[1-5] @Member1 [@Member2] [@Member3] ..."));

    return;
}

void CqAssistantPrivate::groupUnban(const MessageEvent &ev, const QStringList &args)
{
    Q_Q(CqAssistant);

    MasterLevel level = levels->level(ev.from, ev.sender);
    if (level > MasterLevel::Master5) {
        QString name = q->msgAt(ev.sender);
        QString levelName = MasterLevels::levelName(level);
        QString reply = tr("Permission Denied: %1 %2").arg(levelName, name);
        q->sendGroupMessage(ev.from, reply);
        return;
    }

    LevelInfoList ll;
    bool invalidArgs = false;
    for (const QString &arg : args) {
        if (arg.startsWith("[CQ:at")) {
            int i = arg.lastIndexOf("]");
            QString str = arg.mid(10, i - 10);
            ll.append(LevelInfo(str.toLongLong(),
                                MasterLevel::Unknown));
        } else {
            invalidArgs = true;
            break;
        }
    }

    if (!invalidArgs && !ll.isEmpty()) {
        QStringList masters;
        QStringList members;

        levels->update(ev.from, ll);
        for (const LevelInfo &li : ll) {
            if (li.level <= level) {
                masters << q->msgAt(li.uid);
            } else {
                q->banGroupMember(ev.from, li.uid, 0);
                members << q->msgAt(li.uid);
            }
        }

        QStringList reply;
        if (!masters.isEmpty()) {
            reply << tr("Permission Denied:");
            reply << masters;
        }
        if (!members.isEmpty()) {
            reply << tr("Unban List:");
            reply << members;
        }
        q->sendGroupMessage(ev.from, reply.join("\n"));

        return;
    }

    q->sendGroupMessage(ev.from, tr("unban [@Member1] [@Member2] [@Member3] ..."));
}

void CqAssistantPrivate::groupUnkill(const MessageEvent &ev, const QStringList &args)
{
    Q_Q(CqAssistant);

    MasterLevel level = levels->level(ev.from, ev.sender);
    if (level > MasterLevel::Master3) {
        QString name = q->msgAt(ev.sender);
        QString levelName = MasterLevels::levelName(level);
        QString reply = tr("Permission Denied: %1 %2").arg(levelName, name);
        q->sendGroupMessage(ev.from, reply);
        return;
    }

    LevelInfoList ll;
    bool invalidArgs = false;
    for (const QString &arg : args) {

        if (arg.startsWith("[CQ:at")) {
            int i = arg.lastIndexOf("]");
            QString str = arg.mid(10, i - 10);
            ll.append(LevelInfo(str.toLongLong(),
                                MasterLevel::Unknown));
        } else {
            invalidArgs = true;
            break;
        }
    }

    if (!invalidArgs && !ll.isEmpty()) {
        QStringList masters;
        QStringList members;

        levels->update(ev.from, ll);
        for (const LevelInfo &li : ll) {
            if (li.level <= level) {
                masters << q->msgAt(li.uid);
            } else {
                deathHouse->freeMember(ev.from, li.uid);
                members << q->msgAt(li.uid);
            }
        }

        QStringList reply;
        if (!masters.isEmpty()) {
            reply << tr("Permission Denied:");
            reply << masters;
        }
        if (!members.isEmpty()) {
            reply << tr("Unkill List:");
            reply << members;
        }
        q->sendGroupMessage(ev.from, reply.join("\n"));

        return;
    }

    q->sendGroupMessage(ev.from, tr("unkill [@Member1] [@Member2] [@Member3] ..."));
}

void CqAssistantPrivate::groupUnpower(const MessageEvent &ev, const QStringList &args)
{
    Q_Q(CqAssistant);

    MasterLevel level = levels->level(ev.from, ev.sender);
    if (level > MasterLevel::Master1) {
        QString name = q->msgAt(ev.sender);
        QString levelName = MasterLevels::levelName(level);
        QString reply = tr("Permission Denied: %1 %2").arg(levelName, name);
        q->sendGroupMessage(ev.from, reply);
        return;
    }

    bool argvGlobal = false;

    LevelInfoList ll;
    bool invalidArgs = false;
    for (const QString &arg : args) {
        if (arg.startsWith("[CQ:at")) {
            int i = arg.lastIndexOf("]");
            QString str = arg.mid(10, i - 10);
            ll.append(LevelInfo(str.toLongLong(),
                                MasterLevel::Unknown));
        } else if (arg == QLatin1String("g")) {
            argvGlobal = true;
        } else if (arg == QLatin1String("global")) {
            argvGlobal = true;
        } else {
            invalidArgs = true;
            break;
        }
    }

    if (!invalidArgs && !ll.isEmpty()) {
        if (argvGlobal) {
            if (level != MasterLevel::ATField) {
                QString name = q->msgAt(ev.sender);
                QString levelName = MasterLevels::levelName(level);
                QString reply = tr("Permission Denied: %1 %2").arg(levelName, name);
                q->sendGroupMessage(ev.from, reply);
                return;
            }
        }

        QStringList impowers;
        QStringList unchanges;
        QStringList depowers;

        levels->update(ev.from, ll);
        for (const LevelInfo &li : ll) {
            if (li.level <= level) {
                QString name = q->msgAt(ev.sender);
                QString levelName = MasterLevels::levelName(level);
                QString reply = tr("Permission Denied: %1 %2").arg(levelName, name);
                q->sendGroupMessage(ev.from, reply);
                return;
            }
        }

        MasterLevel newLevel = MasterLevel::Unknown;

        for (const LevelInfo &li : ll) {
            levels->setLevel(argvGlobal ? 0 : ev.from, li.uid, newLevel);

            if (newLevel < li.level) {
                impowers << tr("%1 > %2: %3").arg(MasterLevels::levelName(li.level), MasterLevels::levelName(newLevel), q->msgAt(li.uid));
            } else if (newLevel == li.level) {
                unchanges << tr("%1: %3").arg(MasterLevels::levelName(li.level), q->msgAt(li.uid));
            } else {
                depowers << tr("%1 > %2: %3").arg(MasterLevels::levelName(li.level), MasterLevels::levelName(newLevel), q->msgAt(li.uid));
            }
        }

        QStringList reply;
        if (!impowers.isEmpty()) {
            if (argvGlobal) {
                reply << tr("Impower List(Global):");
            } else {
                reply << tr("Impower List:");
            }
            reply << impowers;
        }
        if (!unchanges.isEmpty()) {
            if (argvGlobal) {
                reply << tr("Unchange List(Global):");
            } else {
                reply << tr("Unchange List:");
            }
            reply << unchanges;
        }
        if (!depowers.isEmpty()) {
            if (argvGlobal) {
                reply << tr("Depower List(Global):");
            } else {
                reply << tr("Depower List:");
            }
            reply << depowers;
        }
        q->sendGroupMessage(ev.from, reply.join("\n"));
    }

    q->sendGroupMessage(ev.from, tr("unpower [@Member1] [@Member2] [@Member3] ..."));
}
