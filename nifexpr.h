/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2012, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
   used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#ifndef NIFEXPR_H
#define NIFEXPR_H
#pragma once

#include <QVariant>
#include <QString>

class Expression
{
   enum Operator {
      e_nop, e_not_eq, e_eq, e_gte, e_lte, e_gt, e_lt, e_bit_and, e_bit_or, 
      e_add, e_sub, e_bool_and, e_bool_or, e_not,
   };
   QVariant lhs;
   QVariant rhs;
   Operator opcode;

public:
   explicit Expression()
   {
      opcode = Expression::e_nop;
   }

   Expression(const QString & cond, int startpos, int endpos)
   {
      opcode = Expression::e_nop;
      partition(cond.mid(startpos, endpos-startpos+1));
   }

   Expression(const QString & cond)
   {
      opcode = Expression::e_nop;
      partition(cond);
   }

   QString toString() const;

public:
   template <class F>
   QVariant evaluateValue( const F& convert ) const
   {
      QVariant l = convertValue(lhs, convert);
      QVariant r = convertValue(rhs, convert);
      NormalizeVariants(l, r);

      switch (opcode)
      {
      case Expression::e_not:
         return QVariant::fromValue( !r.toBool() );
      case Expression::e_not_eq:
         return QVariant::fromValue(l != r);
      case Expression::e_eq:
         return QVariant::fromValue(l == r);
      case Expression::e_gte:
         return QVariant::fromValue(l.toUInt() >= r.toUInt());
      case Expression::e_lte:
         return QVariant::fromValue(l.toUInt() <= r.toUInt());
      case Expression::e_gt:
         return QVariant::fromValue(l.toUInt() > r.toUInt());
      case Expression::e_lt:
         return QVariant::fromValue(l.toUInt() < r.toUInt());
      case Expression::e_bit_and:
         return QVariant::fromValue(l.toUInt() & r.toUInt());
      case Expression::e_bit_or:
         return QVariant::fromValue(l.toUInt() | r.toUInt());
      case Expression::e_add:
         return QVariant::fromValue(l.toUInt() + r.toUInt());
      case Expression::e_sub:
         return QVariant::fromValue(l.toUInt() - r.toUInt());
      case Expression::e_bool_and:
         return QVariant::fromValue(l.toBool() && r.toBool());
      case Expression::e_bool_or:
         return QVariant::fromValue(l.toBool() || r.toBool());
      case Expression::e_nop:
         return l;
      }
      return l;
   }

   template <class F>
   bool evaluateBool( const F& convert ) const
   {
      return evaluateValue(convert).toBool();
   }

private:
   static Operator operatorFromString(const QString& str);
   void partition(const QString & cond, int offset = 0);
   void NormalizeVariants(QVariant &l, QVariant &r) const;

   template <class F>
   QVariant convertValue(const QVariant& v, const F& convert ) const
   {
      if (v.type() == QVariant::UserType)
      {
         if ( v.canConvert<Expression>() )
            return v.value<Expression>().evaluateValue(convert);
      }
      return convert(v);
   }
};

Q_DECLARE_METATYPE(Expression)

#endif
