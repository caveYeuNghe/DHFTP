CREATE DATABASE FileSystem
GO

USE FileSystem
GO

CREATE TABLE Account (
	username	varchar(10)	NOT NULL UNIQUE,
	password	varchar(10)	NOT NULL,
	status		bit			NOT NULL
)
GO