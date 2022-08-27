

#include "doomdef.h"
#include "g_game.h"
#include "doomstat.h"


#define SAVEGAMESIZE 0x2c000


// Gamestate the last time G_Ticker was called.

gamestate_t oldgamestate;

gameaction_t gameaction;
gamestate_t gamestate;
skill_t gameskill;
boolean respawnmonsters;
int gameepisode;
int gamemap;

// from here... LATER


//
// G_DoLoadLevel
//
void G_DoLoadLevel(void) {
  int i;

  // Set the sky map.
  // First thing, we have a dummy sky texture name,
  //  a flat. The data is in the WAD only because
  //  we look for an actual index, instead of simply
  //  setting one.

    /* LATER
  skyflatnum = R_FlatNumForName((SKYFLATNAME));

  // The "Sky never changes in Doom II" bug was fixed in
  // the id Anthology version of doom2.exe for Final Doom.
  if ((gamemode == commercial) &&
      (gameversion == exe_final2 || gameversion == exe_chex)) {
    char *skytexturename;

    if (gamemap < 12) {
      skytexturename = "SKY1";
    } else if (gamemap < 21) {
      skytexturename = "SKY2";
    } else {
      skytexturename = "SKY3";
    }

    skytexture = R_TextureNumForName(skytexturename);
  }

  levelstarttic = gametic; // for time calculation

  if (wipegamestate == GS_LEVEL)
    wipegamestate = -1; // force a wipe

  gamestate = GS_LEVEL;

  for (i = 0; i < MAXPLAYERS; i++) {
    turbodetected[i] = false;
    if (playeringame[i] && players[i].playerstate == PST_DEAD)
      players[i].playerstate = PST_REBORN;
    memset(players[i].frags, 0, sizeof(players[i].frags));
  }

  P_SetupLevel(gameepisode, gamemap, 0, gameskill);
  displayplayer = consoleplayer; // view the guy you are playing
  gameaction = ga_nothing;
  Z_CheckHeap();

  // clear cmd building stuff

  memset(gamekeydown, 0, sizeof(gamekeydown));
  joyxmove = joyymove = joystrafemove = 0;
  mousex = mousey = 0;
  sendpause = sendsave = paused = false;
  memset(mousearray, 0, sizeof(mousearray));
  memset(joyarray, 0, sizeof(joyarray));

  if (testcontrols) {
    players[consoleplayer].message = "Press escape to quit.";
  }
*/
}

//
// G_Ticker
// Make ticcmd_ts for the players.
//
void G_Ticker (void) 
{ 
    int		i;
    int		buf; 
    // ticcmd_t*	cmd;
    
    // LATER
    // do player reborns if needed
    // for (i=0 ; i<MAXPLAYERS ; i++) 
	// if (playeringame[i] && players[i].playerstate == PST_REBORN) 
	//     G_DoReborn (i);
    
    // do things to change the game state
    // while (gameaction != ga_nothing) 
    // { 
	// switch (gameaction) 
	// { 
	//   case ga_loadlevel: 
	//     G_DoLoadLevel (); 
	//     break; 
	//   case ga_newgame: 
	//     G_DoNewGame (); 
	//     break; 
	//   case ga_loadgame: 
	//     G_DoLoadGame (); 
	//     break; 
	//   case ga_savegame: 
	//     G_DoSaveGame (); 
	//     break; 
	//   case ga_playdemo: 
	//     G_DoPlayDemo (); 
	//     break; 
	//   case ga_completed: 
	//     G_DoCompleted (); 
	//     break; 
	//   case ga_victory: 
	//     F_StartFinale (); 
	//     break; 
	//   case ga_worlddone: 
	//     G_DoWorldDone (); 
	//     break; 
	//   case ga_screenshot: 
	//     V_ScreenShot("DOOM%02i.%s"); 
    //         players[consoleplayer].message = DEH_String("screen shot");
	//     gameaction = ga_nothing; 
	//     break; 
	//   case ga_nothing: 
	//     break; 
	// } 
    // }
    
    // get commands, check consistancy,
    // and build new consistancy check
    // buf = (gametic/ticdup)%BACKUPTICS; 
 
    // for (i=0 ; i<MAXPLAYERS ; i++)
    // {
	// if (playeringame[i]) 
	// { 
	//     cmd = &players[i].cmd; 

	//     memcpy(cmd, &netcmds[i], sizeof(ticcmd_t));

	//     if (demoplayback) 
	// 	G_ReadDemoTiccmd (cmd); 
	//     if (demorecording) 
	// 	G_WriteDemoTiccmd (cmd);
	    
	//     // check for turbo cheats

    //         // check ~ 4 seconds whether to display the turbo message. 
    //         // store if the turbo threshold was exceeded in any tics
    //         // over the past 4 seconds.  offset the checking period
    //         // for each player so messages are not displayed at the
    //         // same time.

    //         if (cmd->forwardmove > TURBOTHRESHOLD)
    //         {
    //             turbodetected[i] = true;
    //         }

    //         if ((gametic & 31) == 0 
    //          && ((gametic >> 5) % MAXPLAYERS) == i
    //          && turbodetected[i])
    //         {
    //             static char turbomessage[80];
    //             extern char *player_names[4];
    //             M_snprintf(turbomessage, sizeof(turbomessage),
    //                        "%s is turbo!", player_names[i]);
    //             players[consoleplayer].message = turbomessage;
    //             turbodetected[i] = false;
    //         }

	//     if (netgame && !netdemo && !(gametic%ticdup) ) 
	//     { 
	// 	if (gametic > BACKUPTICS 
	// 	    && consistancy[i][buf] != cmd->consistancy) 
	// 	{ 
	// 	    I_Error ("consistency failure (%i should be %i)",
	// 		     cmd->consistancy, consistancy[i][buf]); 
	// 	} 
	// 	if (players[i].mo) 
	// 	    consistancy[i][buf] = players[i].mo->x; 
	// 	else 
	// 	    consistancy[i][buf] = rndindex; 
	//     } 
	// }
    // }
    
    // // check for special buttons
    // for (i=0 ; i<MAXPLAYERS ; i++)
    // {
	// if (playeringame[i]) 
	// { 
	//     if (players[i].cmd.buttons & BT_SPECIAL) 
	//     { 
	// 	switch (players[i].cmd.buttons & BT_SPECIALMASK) 
	// 	{ 
	// 	  case BTS_PAUSE: 
	// 	    paused ^= 1; 
	// 	    if (paused) 
	// 		S_PauseSound (); 
	// 	    else 
	// 		S_ResumeSound (); 
	// 	    break; 
					 
	// 	  case BTS_SAVEGAME: 
	// 	    if (!savedescription[0]) 
    //                 {
    //                     M_StringCopy(savedescription, "NET GAME",
    //                                  sizeof(savedescription));
    //                 }

	// 	    savegameslot =  
	// 		(players[i].cmd.buttons & BTS_SAVEMASK)>>BTS_SAVESHIFT; 
	// 	    gameaction = ga_savegame; 
	// 	    break; 
	// 	} 
	//     } 
	// }
    // }

    // // Have we just finished displaying an intermission screen?

    // if (oldgamestate == GS_INTERMISSION && gamestate != GS_INTERMISSION)
    // {
    //     WI_End();
    // }

    // oldgamestate = gamestate;
    
    // // do main actions
    // switch (gamestate) 
    // { 
    //   case GS_LEVEL: 
	// P_Ticker (); 
	// ST_Ticker (); 
	// AM_Ticker (); 
	// HU_Ticker ();            
	// break; 
	 
    //   case GS_INTERMISSION: 
	// WI_Ticker (); 
	// break; 
			 
    //   case GS_FINALE: 
	// F_Ticker (); 
	// break; 
 
    //   case GS_DEMOSCREEN: 
	// D_PageTicker (); 
	// break;
    // }        
} 