/*-------------------------------------------------------------------------
 *
 * catname.h
 *	  POSTGRES system catalog relation name definitions.
 *
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/catalog/catname.h,v 1.34 2004/12/31 22:03:24 pgsql Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef CATNAME_H
#define CATNAME_H


#define  AggregateRelationName "pg_aggregate"
#define  AccessMethodRelationName "pg_am"
#define  AccessMethodOperatorRelationName "pg_amop"
#define  AccessMethodProcedureRelationName "pg_amproc"
#define  AttributeRelationName "pg_attribute"
#define  CastRelationName "pg_cast"
#define  ConstraintRelationName "pg_constraint"
#define  ConversionRelationName "pg_conversion"
#define  DatabaseRelationName "pg_database"
#define  DependRelationName "pg_depend"
#define  DescriptionRelationName "pg_description"
#define  GroupRelationName "pg_group"
#define  IndexRelationName "pg_index"
#define  InheritsRelationName "pg_inherits"
#define  LanguageRelationName "pg_language"
#define  LargeObjectRelationName "pg_largeobject"
#define  ListenerRelationName "pg_listener"
#define  NamespaceRelationName "pg_namespace"
#define  OperatorClassRelationName "pg_opclass"
#define  OperatorRelationName "pg_operator"
#define  ProcedureRelationName "pg_proc"
#define  RelationRelationName "pg_class"
#define  RewriteRelationName "pg_rewrite"
#define  ShadowRelationName "pg_shadow"
#define  StatisticRelationName "pg_statistic"
#define  TableSpaceRelationName "pg_tablespace"
#define  TypeRelationName "pg_type"
#define  VersionRelationName "pg_version"
#define  AttrDefaultRelationName "pg_attrdef"
#define  TriggerRelationName "pg_trigger"

#endif   /* CATNAME_H */
