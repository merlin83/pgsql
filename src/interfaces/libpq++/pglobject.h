/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pglobject.h
 *
 *   DESCRIPTION
 *      declaration of the PGlobj class.
 *   PGlobj encapsulates a large object interface to Postgres backend 
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 *  $Id: pglobject.h,v 1.2 1999-05-23 01:04:03 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
 
#ifndef PGLOBJ_H
#define PGLOBJ_H

#include "pgconnection.h"

// buffer size
#define BUFSIZE 1024


// ****************************************************************
//
// PgLargeObject - a class for accessing Large Object in a database
//
// ****************************************************************
class PgLargeObject : public PgConnection {
private:
  int pgFd;
  Oid pgObject;
  string loStatus;

public:
  PgLargeObject(const char* conninfo = 0);   // use reasonable defaults and create large object
  PgLargeObject(Oid lobjId, const char* conninfo = 0); // use reasonable defaults and open large object
  ~PgLargeObject(); // close connection and clean up
  
  void Create();
  void Open();
  void Close();
  int Read(char* buf, int len);
  int Write(const char* buf, int len);
  int LSeek(int offset, int whence);
  int Tell();
  int Unlink();
  Oid LOid();
  Oid Import(const char* filename);
  int Export(const char* filename); 
  string Status();
  
private:
   void Init(Oid lobjId = 0);
};

#endif	// PGLOBJ_H

// sig 11's if the filename points to a binary file.
