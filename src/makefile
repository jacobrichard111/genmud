CFLAGS = -c -Wall -pedantic -g
LFLAGS = -lpthread -lcrypt


OBJS =  aff.o alias.o areagen.o areagen2.o auction.o bank.o battle.o \
	boot.o cavegen.o chan.o citygen.o citygen2.o \
	clan.o consid.o const.o color.o  combatai.o \
	craft.o create.o \
	dam.o detailgen.o \
	disaster.o edit.o event.o fight.o \
	fileio.o follow.o \
	guild.o help.o historygen.o implant.o info.o \
	int.o kde.o look.o magic.o magicitem.o map.o mapgen.o \
	mobgen.o money.o move.o myrand.o note.o objectgen.o \
	parse.o pcio.o pkdata.o plasmgen.o \
	prac.o proc.o provisiongen.o \
	questgen.o questgenold.o quit.o race.o randobj.o roomdescgen.o \
	roomgen.o rumor.o \
	script.o scriptio.o scriptedit.o scriptline.o security.o serv.o \
	shop.o socials.o socibuild.o sociedit.o society.o societyio.o \
	socifight.o socirun.o socirts.o socigen.o sociname.o socineed.o \
	sociraid.o sociplay.o socireward.o socidiplomat.o \
	spell.o stredit.o stringgen.o stringparse.o thing.o thingio.o \
	track.o upd.o wear.o weather.o worldgen.o worldgen2.o\
#	wilderness.o wildalife.o \
#	THE PRECEEDING LINE IS NOT ADDED IN UNLESS YOU REMOVE THE #
#	ONLY USE THE PRECEEDING LINE IF YOU WANT THE WILDERNESS MAP!!!

CC = gcc



.c.o: serv.h
	$(CC) $(CFLAGS) $<

mud: $(OBJS)
	rm -f mud
	$(CC) $(LFLAGS) -o mud $(OBJS)

clean:
	\rm -rf *.o mud
