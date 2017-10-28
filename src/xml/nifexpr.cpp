/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
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


//! @file nifexpr.cpp Expression parsing for conditions defined in nif.xml.

static bool matchGroup( const std::string & cond, int offset, int & startpos, int & endpos )
{
	int scandepth = 0;
	startpos = -1;
	endpos = -1;

	for ( int scanpos = offset, len = cond.length(); scanpos != len; ++scanpos ) {
		switch ( cond[scanpos] )
		{
		case '(':
			if ( startpos == -1 )
				startpos = scanpos;

			++scandepth;
			break;
		case ')':
			if ( --scandepth == 0 ) {
				endpos = scanpos;
				return true;
			}
			break;
		}
	}

	if ( startpos != -1 || endpos != -1 )
		throw "expression syntax error (non-matching brackets?)";

	return false;
}


static quint32 version2number( const QString & s )
{
	if ( s.isEmpty() )
		return 0;

	if ( s.contains( "." ) ) {
		QStringList l = s.split( "." );

		quint32 v = 0;

		if ( l.count() > 4 ) {
			// Should probaby post a warning here or something.  Version # has more than 3 dots in it.
			return 0;
		} else if ( l.count() == 2 ) {
			// This is an old style version number.  Take each digit following the first one at a time.
			// The first one is the major version
			v += l[0].toInt() << (3 * 8);

			if ( l[1].size() >= 1 ) {
				v += l[1].mid( 0, 1 ).toInt() << (2 * 8);
			}

			if ( l[1].size() >= 2 ) {
				v += l[1].mid( 1, 1 ).toInt() << (1 * 8);
			}

			if ( l[1].size() >= 3 ) {
				v += l[1].mid( 2, -1 ).toInt();
			}

			return v;
		}

		// This is a new style version number with dots separating the digits
		for ( int i = 0; i < 4 && i < l.count(); i++ ) {
			v += l[i].toInt( 0, 10 ) << ( (3 - i) * 8 );
		}

		return v;
	}

	bool ok;
	quint32 i = s.toUInt( &ok );
	return ( i == 0xffffffff ? 0 : i );
}

NifExpr::Operator NifExpr::operatorFromString( const QString & str )
{
	if ( str == "!" )
		return NifExpr::e_not;
	else if ( str == "!=" )
		return NifExpr::e_not_eq;
	else if ( str == "==" )
		return NifExpr::e_eq;
	else if ( str == ">=" )
		return NifExpr::e_gte;
	else if ( str == "<=" )
		return NifExpr::e_lte;
	else if ( str == ">" )
		return NifExpr::e_gt;
	else if ( str == "<" )
		return NifExpr::e_lt;
	else if ( str == "&" )
		return NifExpr::e_bit_and;
	else if ( str == "|" )
		return NifExpr::e_bit_or;
	else if ( str == "+" )
		return NifExpr::e_add;
	else if ( str == "-" )
		return NifExpr::e_sub;
	else if ( str == "/" )
		return NifExpr::e_div;
	else if ( str == "*" )
		return NifExpr::e_mul;
	else if ( str == "&&" )
		return NifExpr::e_bool_and;
	else if ( str == "||" )
		return NifExpr::e_bool_or;

	return NifExpr::e_nop;
}

void NifExpr::partition( const QString & cond, int offset /*= 0*/ )
{
	int pos;

	if ( cond.isEmpty() ) {
		opcode = NifExpr::e_nop;
		return;
	}

	// Handle unary operators
	QRegularExpression reUnary( "^\\s*!(.*)" );
	QRegularExpressionMatch reUnaryMatch = reUnary.match( cond, offset );
	pos = reUnaryMatch.capturedStart();
	if ( pos != -1 ) {
		NifExpr e( reUnaryMatch.captured( 1 ).trimmed() );
		opcode = NifExpr::e_not;
		rhs = QVariant::fromValue( e );
		return;
	}

	int lstartpos = -1, lendpos = -1, // Left Start/End
		ostartpos = -1, oendpos = -1, // Operator Start/End
		rstartpos = -1, rendpos = -1; // Right Start/End

	QRegularExpression reOps( "(!=|==|>=|<=|>|<|\\+|-|/|\\*|\\&\\&|\\|\\||\\&|\\|)" );
	QRegularExpression reLParen( "^\\s*\\(.*" );

	QRegularExpressionMatch reLParenMatch = reLParen.match( cond, offset );

	// Check for left group
	pos = reLParenMatch.capturedStart();
	if ( pos != -1 ) {
		// Get start/end pos for lparen
		matchGroup( cond.toStdString(), pos, lstartpos, lendpos );
		// Find operator in group
		QRegularExpressionMatch reOpsMatch = reOps.match( cond, lendpos + 1 );
		pos = reOpsMatch.capturedStart();
		// Move positions inward
		++lstartpos, --lendpos;

		if ( pos != -1 ) {
			ostartpos = pos;
			oendpos = ostartpos + reOpsMatch.captured( 0 ).length();
		} else {
			partition( cond.mid( lstartpos, lendpos - lstartpos + 1 ) );
			return;
		}
	} else {
		// Check for expression without parens
		QRegularExpressionMatch reOpsMatch = reOps.match( cond, offset );
		pos = reOpsMatch.capturedStart();
		if ( pos != -1 ) {
			lstartpos = offset;
			lendpos = pos - 1;
			ostartpos = pos;
			oendpos = ostartpos + reOpsMatch.captured( 0 ).length();
		} else {
			static QRegularExpression reInt( "\\A(?:[-+]?[0-9]+)\\z" );
			static QRegularExpression reUInt( "\\A(?:0[xX][0-9a-fA-F]+)\\z" );
			static QRegularExpression reFloat( "^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$" );
			static QRegularExpression reVersion( "\\A(?:[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)\\z" );

			// termination
			lhs.setValue( cond );

			if ( reUInt.match( cond ).hasMatch() ) {
				bool ok = false;
				lhs.setValue( cond.toUInt( &ok, 16 ) );
				lhs.convert( QVariant::UInt );
			} else if ( reInt.match( cond ).hasMatch() ) {
				lhs.convert( QVariant::Int );
			} else if ( reVersion.match( cond ).hasMatch() ) {
				lhs.setValue( version2number( cond ) );
			}

			opcode = NifExpr::e_nop;
			return;
		}
	}

	rstartpos = oendpos + 1;
	rendpos = cond.size() - 1;

	NifExpr lhsexp( cond.mid( lstartpos, lendpos - lstartpos + 1 ).trimmed() );
	NifExpr rhsexp( cond.mid( rstartpos, rendpos - rstartpos + 1 ).trimmed() );

	if ( lhsexp.opcode == NifExpr::e_nop ) {
		lhs = lhsexp.lhs;
	} else {
		lhs = QVariant::fromValue( lhsexp );
	}

	opcode = operatorFromString( cond.mid( ostartpos, oendpos - ostartpos ) );

	if ( rhsexp.opcode == NifExpr::e_nop ) {
		rhs = rhsexp.lhs;
	} else {
		rhs = QVariant::fromValue( rhsexp );
	}
}

QString NifExpr::toString() const
{
	QString l = lhs.toString();
	QString r = rhs.toString();

	if ( lhs.type() == QVariant::UserType && lhs.canConvert<NifExpr>() )
		l = lhs.value<NifExpr>().toString();

	if ( rhs.type() == QVariant::UserType && rhs.canConvert<NifExpr>() )
		r = rhs.value<NifExpr>().toString();

	switch ( opcode ) {
	case NifExpr::e_not:
		return QString( "!%1" ).arg( r );
	case NifExpr::e_not_eq:
		return QString( "(%1 != %2)" ).arg( l, r );
	case NifExpr::e_eq:
		return QString( "(%1 == %2)" ).arg( l, r );
	case NifExpr::e_gte:
		return QString( "(%1 >= %2)" ).arg( l, r );
	case NifExpr::e_lte:
		return QString( "(%1 <= %2)" ).arg( l, r );
	case NifExpr::e_gt:
		return QString( "(%1 > %2)" ).arg( l, r );
	case NifExpr::e_lt:
		return QString( "(%1 < %2)" ).arg( l, r );
	case NifExpr::e_bit_and:
		return QString( "(%1 & %2)" ).arg( l, r );
	case NifExpr::e_bit_or:
		return QString( "(%1 | %2)" ).arg( l, r );
	case NifExpr::e_add:
		return QString( "(%1 + %2)" ).arg( l, r );
	case NifExpr::e_sub:
		return QString( "(%1 - %2)" ).arg( l, r );
	case NifExpr::e_div:
		return QString( "(%1 / %2)" ).arg( l, r );
	case NifExpr::e_mul:
		return QString( "(%1 * %2)" ).arg( l, r );
	case NifExpr::e_bool_and:
		return QString( "(%1 && %2)" ).arg( l, r );
	case NifExpr::e_bool_or:
		return QString( "(%1 || %2)" ).arg( l, r );
	case NifExpr::e_nop:
		return QString( "%1" ).arg( l );
	}

	return QString();
}

void NifExpr::NormalizeVariants( QVariant & l, QVariant & r ) const
{
	if ( l.isValid() && r.isValid() ) {
		if ( l.type() != r.type() ) {
			if ( l.type() == QVariant::String && l.canConvert( r.type() ) )
				l.convert( r.type() );
			else if ( r.type() == QVariant::String && r.canConvert( l.type() ) )
				r.convert( l.type() );
			else {
				QVariant::Type t = l.type() > r.type() ? l.type() : r.type();

				if ( r.canConvert( t ) && l.canConvert( t ) ) {
					l.convert( t );
					r.convert( t );
				}
			}
		}
	}
}
