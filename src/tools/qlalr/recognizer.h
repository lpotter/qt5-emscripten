/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QLALR project on Qt Labs.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "grammar_p.h"

#include "lalr.h"

#include <QtCore/qdebug.h>
#include <QtCore/qstring.h>
#include <QtCore/qfile.h>
#include <QtCore/qtextstream.h>

#include <cstdlib>

class Recognizer: protected grammar
{
public:
  Recognizer (Grammar *grammar, bool no_lines);
  ~Recognizer();

  bool parse (const QString &input_file = QString ());

  inline QString decls () const { return _M_decls; }
  inline QString impls () const { return _M_impls; }

protected:
  inline void reallocateStack ();

  inline QString &sym (int index)
  { return sym_stack [tos + index - 1]; }

protected: // scanner
  int nextToken();

  inline void inp ()
  {
    if (_M_currentChar != _M_lastChar)
      {
        ch = *_M_currentChar++;

        if (ch == QLatin1Char('\n'))
          ++_M_line;
      }
    else
      ch = QChar();
  }

  QString expand (const QString &text) const;

protected:
  // recognizer
  int tos;
  int stack_size;
  QVector<QString> sym_stack;
  int *state_stack;

  QString _M_contents;
  QString::const_iterator _M_firstChar;
  QString::const_iterator _M_lastChar;
  QString::const_iterator _M_currentChar;

  // scanner
  QChar ch;
  int _M_line;
  int _M_action_line;
  Grammar *_M_grammar;
  RulePointer _M_current_rule;
  QString _M_input_file;

  QString _M_decls;
  QString _M_impls;
  QString _M_current_value;
  bool _M_no_lines;
};
