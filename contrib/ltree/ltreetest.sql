create table test ( path ltree);
insert into test values ('Top');
insert into test values ('Top.Science');
insert into test values ('Top.Science.Astronomy');
insert into test values ('Top.Science.Astronomy.Astrophysics');
insert into test values ('Top.Science.Astronomy.Cosmology');
insert into test values ('Top.Hobbies');
insert into test values ('Top.Hobbies.Amateurs_Astronomy');
insert into test values ('Top.Collections');
insert into test values ('Top.Collections.Pictures');
insert into test values ('Top.Collections.Pictures.Astronomy');
insert into test values ('Top.Collections.Pictures.Astronomy.Stars');
insert into test values ('Top.Collections.Pictures.Astronomy.Galaxies');
insert into test values ('Top.Collections.Pictures.Astronomy.Astronauts');
create index path_gist_idx on test using gist(path);
create index path_idx on test using btree(path);
