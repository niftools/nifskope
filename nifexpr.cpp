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

#include "nifexpr.h"
#include "basemodel.h"

static bool matchGroup(const QString & cond, int offset, int& startpos, int& endpos)
{
   int scandepth = 0;
   startpos = -1;
   endpos = -1;
   for (int scanpos = offset, len=cond.length(); scanpos != len; ++scanpos) {
      QChar c = cond[scanpos];
      if (c == '(') {
         if (startpos == -1)
            startpos = scanpos;
         ++scandepth;
      } else if ( c == ')' ) {
         if (--scandepth == 0) {
            endpos = scanpos;
            return true;
         }
      }
   }
   if (startpos != -1 || endpos != -1)
      throw "expression syntax error (non-matching brackets?)";
   return false;
}


static quint32 version2number( const QString & s )
{
   if ( s.isEmpty() )	return 0;

   if ( s.contains( "." ) )
   {
      QStringList l = s.split( "." );

      quint32 v = 0;

      if ( l.count() > 4 ) {
         //Should probaby post a warning here or something.  Version # has more than 3 dots in it.
         return 0;
      } else if ( l.count() == 2 ) {
         //This is an old style version number.  Take each digit following the first one at a time.
         //The first one is the major version
         v += l[0].toInt() << (3 * 8);

         if ( l[1].size() >= 1 ) {
            v += l[1].mid(0, 1).toInt() << (2 * 8);
         }
         if ( l[1].size() >= 2 ) {
            v += l[1].mid(1, 1).toInt() << (1 * 8);
         }
         if ( l[1].size() >= 3 ) {
            v += l[1].mid(2, -1).toInt();
         }
         return v;
      } else {
         //This is a new style version number with dots separating the digits
         for ( int i = 0; i < 4 && i < l.count(); i++ ) {
            v += l[i].toInt( 0, 10 ) << ( (3-i) * 8 );
         }
         return v;
      }

   } else {
      bool ok;
      quint32 i = s.toUInt( &ok );
      return ( i == 0xffffffff ? 0 : i );
   }
}

Expression::Operator Expression::operatorFromString( const QString& str )
{
   if ( str == "!" ) return Expression::e_not;
   else if ( str == "!=" ) return Expression::e_not_eq;
   else if ( str == "==" ) return Expression::e_eq;
   else if ( str == ">=" ) return Expression::e_gte;
   else if ( str == "<=" ) return Expression::e_lte;
   else if ( str == ">" ) return Expression::e_gt;
   else if ( str == "<" ) return Expression::e_lt;
   else if ( str == "&" ) return Expression::e_bit_and;
   else if ( str == "|" ) return Expression::e_bit_or;
   else if ( str == "+" ) return Expression::e_add;
   else if ( str == "-" ) return Expression::e_sub;
   else if ( str == "&&" ) return Expression::e_bool_and;
   else if ( str == "||" ) return Expression::e_bool_or;
   return Expression::e_nop;
}

void Expression::partition( const QString & cond, int offset /*= 0*/ )
{
   int pos;
   if (cond.isEmpty())
   {
      this->opcode = Expression::e_nop;
      return;
   }

   // Handle unary operators
   QRegExp reUnary("^\\s*!(.*)");
   pos = reUnary.indexIn(cond, offset, QRegExp::CaretAtOffset);
   if (pos != -1) {
      Expression e(reUnary.cap(1).trimmed());
      this->opcode = Expression::e_not;
      this->rhs = QVariant::fromValue( e );
      return;
   }
   // Check for left group
   int lstartpos=-1, lendpos=-1, ostartpos=-1, oendpos=-1, rstartpos=-1, rendpos=-1;
   //QRegExp tokens("\b(!=|==|>=|<=|>|<|\\&|\+|-|\\&\\&|\\|\\||\(|\)|[a-zA-Z0-9][a-zA-Z0-9_ \\?]*[a-zA-Z0-9_\\?]?)\b");
   QRegExp reOps("(!=|==|>=|<=|>|<|\\&|\\+|-|\\&\\&|\\|\\|)");
   QRegExp reLParen("^\\s*\\(.*");
   pos = reLParen.indexIn(cond,offset, QRegExp::CaretAtOffset);
   if (pos != -1) {
      matchGroup(cond, pos, lstartpos, lendpos);
      pos = reOps.indexIn(cond, lendpos+1, QRegExp::CaretAtOffset);
      ++lstartpos, --lendpos;
      if (pos != -1) {
         ostartpos = pos;
         oendpos = ostartpos + reOps.cap(0).length();
      } else {
         partition(cond.mid(lstartpos, lendpos-lstartpos+1));
         return;
      }
   } else {
      pos = reOps.indexIn(cond, offset, QRegExp::CaretAtOffset);
      if (pos != -1) {
         lstartpos = offset;
         lendpos = pos - 1;
         ostartpos = pos;
         oendpos = ostartpos + reOps.cap(0).length();
      } else {
         static QRegExp reInt("[-+]?[0-9]+");
         static QRegExp reUInt("0[xX][0-9]+");
         static QRegExp reFloat("^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$");
         static QRegExp reVersion("[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+");

         // termination
         this->lhs.setValue(cond);
         if (reUInt.exactMatch(cond)) {
            this->lhs.convert(QVariant::UInt);
         } else if (reInt.exactMatch(cond)) {
            this->lhs.convert(QVariant::Int);
         } else if (reVersion.exactMatch(cond)) {
            this->lhs.setValue( version2number(cond) );
         }
         this->opcode = Expression::e_nop;
         return;
      }
   }

   rstartpos = oendpos+1;
   rendpos = cond.size() - 1;

   Expression lhsexp(cond.mid(lstartpos, lendpos-lstartpos+1).trimmed());
   Expression rhsexp(cond.mid(rstartpos, rendpos-rstartpos+1).trimmed());
   if (lhsexp.opcode == Expression::e_nop) {
      this->lhs = lhsexp.lhs;
   } else {
      this->lhs = QVariant::fromValue( lhsexp );
   }
   this->opcode = operatorFromString(cond.mid(ostartpos, oendpos-ostartpos));
   if (rhsexp.opcode == Expression::e_nop) {
      this->rhs = rhsexp.lhs;
   } else {
      this->rhs = QVariant::fromValue( rhsexp );
   }
}

QString Expression::toString() const
{
   QString l = lhs.toString();
   QString r = rhs.toString();
   if (lhs.type() == QVariant::UserType && lhs.canConvert<Expression>())
      l = lhs.value<Expression>().toString();
   if (rhs.type() == QVariant::UserType && rhs.canConvert<Expression>())
      r = rhs.value<Expression>().toString();

   switch (opcode)
   {
   case Expression::e_not:
      return QString("!%1").arg(r);
   case Expression::e_not_eq:
      return QString("(%1 != %2)").arg(l).arg(r);
   case Expression::e_eq:
      return QString("(%1 == %2)").arg(l).arg(r);
   case Expression::e_gte:
      return QString("(%1 >= %2)").arg(l).arg(r);
   case Expression::e_lte:
      return QString("(%1 <= %2)").arg(l).arg(r);
   case Expression::e_gt:
      return QString("(%1 > %2)").arg(l).arg(r);
   case Expression::e_lt:
      return QString("(%1 < %2)").arg(l).arg(r);
   case Expression::e_bit_and:
      return QString("(%1 & %2)").arg(l).arg(r);
   case Expression::e_bit_or:
      return QString("(%1 | %2)").arg(l).arg(r);
   case Expression::e_add:
      return QString("(%1 + %2)").arg(l).arg(r);
   case Expression::e_sub:
      return QString("(%1 - %2)").arg(l).arg(r);
   case Expression::e_bool_and:
      return QString("(%1 && %2)").arg(l).arg(r);
   case Expression::e_bool_or:
      return QString("(%1 || %2)").arg(l).arg(r);
   case Expression::e_nop:
      return QString("%1").arg(l);
   }
   return QString();
}

void Expression::NormalizeVariants( QVariant &l, QVariant &r ) const
{
	if (l.isValid() && r.isValid()) {
		if (l.type() != r.type()) {
			if ( l.type() == QVariant::String && l.canConvert(r.type()) )
				l.convert(r.type());
			else if ( r.type() == QVariant::String && r.canConvert(l.type()) )
				r.convert(l.type());
			else
			{
				QVariant::Type t = l.type() > r.type() ? l.type() : r.type();
				if (r.canConvert(t) && l.canConvert(t)) {
					l.convert(t);
					r.convert(t);
				}
			}
		}
	}
}
