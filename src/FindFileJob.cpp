/* This file is part of RTags.

RTags is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTags is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTags.  If not, see <http://www.gnu.org/licenses/>. */

#include "FindFileJob.h"
#include "RTags.h"
#include "Server.h"
#include "FileManager.h"
#include "Project.h"

FindFileJob::FindFileJob(const std::shared_ptr<QueryMessage> &query, const std::shared_ptr<Project> &project)
    : QueryJob(query, QuietJob, project)
{
    const String q = query->query();
    if (!q.isEmpty()) {
        if (query->flags() & QueryMessage::MatchRegexp) {
            mRegExp = q;
        } else {
            mPattern = q;
        }
    }
}

int FindFileJob::execute()
{
    std::shared_ptr<Project> proj = project();
    if (!proj || !proj->fileManager) {
        return 1;
    }
    const Path srcRoot = proj->path();
    assert(srcRoot.endsWith('/'));

    enum Mode {
        All,
        FilePath,
        RegExp,
        Pattern,
    } mode = All;
    String::CaseSensitivity cs = String::CaseSensitive;
    if (mRegExp.isValid()) {
        mode = RegExp;
    } else if (!mPattern.isEmpty()) {
        mode = mPattern[0] == '/' ? FilePath : Pattern;
    }
    if (queryFlags() & QueryMessage::MatchCaseInsensitive)
        cs = String::CaseInsensitive;

    String out;
    out.reserve(PATH_MAX);
    const bool absolutePath = queryFlags() & QueryMessage::AbsolutePath;
    if (absolutePath)
        out.append(srcRoot);
    const Files& dirs = proj->files();
    Files::const_iterator dirit = dirs.begin();
    bool foundExact = false;
    const int patternSize = mPattern.size();
    List<String> matches;
    const bool preferExact = queryFlags() & QueryMessage::FindFilePreferExact;
    int ret = 1;
    while (dirit != dirs.end()) {
        const Path &dir = dirit->first;
        if (dir.size() < srcRoot.size()) {
            continue;
        } else {
            out.append(dir.constData() + srcRoot.size(), dir.size() - srcRoot.size());
        }

        const Set<String> &files = dirit->second;
        for (Set<String>::const_iterator it = files.begin(); it != files.end(); ++it) {
            const String &key = *it;
            out.append(key);
            bool ok;
            switch (mode) {
            case All:
                ok = true;
                break;
            case RegExp:
                ok = mRegExp.indexIn(out) != -1;
                break;
            case FilePath:
            case Pattern:
                if (!preferExact) {
                    ok = out.contains(mPattern, cs);
                } else {
                    const int outSize = out.size();
                    const bool exact = (outSize > patternSize && out.endsWith(mPattern) && out.at(outSize - (patternSize + 1)) == '/');
                    if (exact) {
                        ok = true;
                        if (!foundExact) {
                            matches.clear();
                            foundExact = true;
                        }
                    } else {
                        ok = !foundExact && out.contains(mPattern, cs);
                    }
                }
                if (!ok && mode == FilePath) {
                    Path p(out);
                    if (!absolutePath)
                        p.prepend(srcRoot);
                    p.resolve();
                    if (p == mPattern)
                        ok = true;
                }
                break;
            }
            if (ok) {
                ret = 0;

                Path matched = out;
                if (absolutePath)
                    matched.resolve();
                if (preferExact && !foundExact) {
                    matches.append(matched);
                } else if (!write(matched)) {
                    return 1; // ???
                }
            }
            out.chop(key.size());
        }
        out.chop(dir.size() - srcRoot.size());
        ++dirit;
    }
    for (List<String>::const_iterator it = matches.begin(); it != matches.end(); ++it) {
        if (!write(*it)) {
            ret = 2;
            break;
        }
    }
    return ret;
}
