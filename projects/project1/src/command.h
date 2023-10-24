#ifndef COMMAND_H
#define COMMAND_H

int command_roll(Command* command);
int command_cd(Command* command, cdhistory* hist);
int command_cdh(Command* command, cdhistory* hist);
int command_ls(Command* command);
int command_cloc(Command* command);
int command_psvis(Command* command);

#endif
