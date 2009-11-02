#include <libgen.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <spotify/api.h>


/*
	Kinda stupid way of configuring stuff, but I'm to lazy to fix fetching from
 	arguments.. Did i mention that I fucking hate C ?

 */
const char *playlist_name = "Radio 1";
const char *playlist_file = "radio1.pls";

extern const uint8_t g_appkey[];
extern const size_t g_appkey_size;
int g_exit_code = -1;
static pthread_t g_main_thread = -1;
static sp_session *g_session;
static int got_callback = 0;
static int update_running = 0;

void session_ready(sp_session *session)
{
	g_session = session;
}

static void log_message(sp_session *session, const char *data)
{
	fprintf(stderr, "log_message: %s\n", data);
}


static void connection_error(sp_session *session, sp_error error)
{
	fprintf(stderr, "connection to Spotify failed: %s\n",
	                sp_error_message(error));
	g_exit_code = 5;
}

static void logged_in(sp_session *session, sp_error error)
{
	if (SP_ERROR_OK != error) {
		fprintf(stderr, "failed to log in to Spotify: %s\n",
		                sp_error_message(error));
		g_exit_code = 4;
		return;
	}

	// Let us print the nice message...
	sp_user *me = sp_session_user(session);
	const char *my_name = (sp_user_is_loaded(me) ?
		sp_user_display_name(me) :
		sp_user_canonical_name(me));

	printf("Logged in to Spotify as user %s\n", my_name);

	session_ready(session);
}

/**
 * This callback is called when the session has logged out of Spotify.
 *
 * @sa sp_session_callbacks#logged_out
 */
static void logged_out(sp_session *session)
{
	if (g_exit_code < 0)
		g_exit_code = 0;
}

static void print_track(sp_track *track)
{
	int duration = sp_track_duration(track);

	printf("  Track \"%s\" [%d:%02d] has %d artist(s), %d%% popularity\n",
		sp_track_name(track),
		duration / 60000,
		(duration / 1000) / 60,
		sp_track_num_artists(track),
		sp_track_popularity(track));
}

static void terminate(void)
{
	sp_error error;

	error = sp_session_logout(g_session);

	if (SP_ERROR_OK != error) {
		fprintf(stderr, "failed to log out from Spotify: %s\n",
		                sp_error_message(error));
		g_exit_code = 5;
		return;
	}
}




static void wait_for_pl_ack(char* data){
	printf("Got callback: %s\n",data);
	got_callback = 1;
}
static sp_playlist_callbacks sp_callbacks = {
	&wait_for_pl_ack
	};
void update_playlists(sp_session *session) {
	log_message(session, "Starting playlist update");
	update_running = 1;
	// Update playlists
	sp_playlistcontainer *pc = sp_session_playlistcontainer(session);
	int found = -1;
	int i;
	for (i = 0; i < sp_playlistcontainer_num_playlists(pc); i++) {
		sp_playlist *tpl = sp_playlistcontainer_playlist(pc, i);
		const char *name = sp_playlist_name(tpl);
		if (strcmp(name, playlist_name) == 0){
			found = i;
		}
	}
	sp_playlist *pl = NULL;

	if (found == -1){
		printf("Creating playlist %s\n", playlist_name);
		pl = sp_playlistcontainer_add_new_playlist(pc, playlist_name);
		if (pl == NULL) {
			fprintf(stderr, "failed to create playlist\n");
			return;
		}
	} else {
		fprintf(stderr, "Fetching existing playlist at index %d\n", found);
		pl = sp_playlistcontainer_playlist(pc, found);
		if (pl == NULL){
			fprintf(stderr, "failed to fetch playlist\n");
			return;
		}
	}
	fprintf(stdout, "Playlist: %i\n", pl == NULL ? 1: 2);
	if (found != -1 && sp_playlist_num_tracks(pl) > 0) {
		fprintf(stdout, "Clearing playlist %s\n", sp_playlist_name(pl));
		fprintf(stdout, "Num tracks: %d\n", sp_playlist_num_tracks(pl));
		int k = 0;
		int *tracks = (int *) malloc (sizeof(int)*sp_playlist_num_tracks(pl));
		for(k = sp_playlist_num_tracks(pl) -1 ; k >= 0; k--){
			tracks[k] = k;
		}
		sp_playlist_remove_tracks(pl, tracks, sp_playlist_num_tracks(pl)-1);
	}

	FILE *fp = fopen(playlist_file, "r");
	const sp_track *new_tracks[256];
	char buffer [256];
	int j = 0;
	while (j < 255 && fgets(buffer, 37, fp) != NULL){
		sp_link *link = sp_link_create_from_string(buffer);
		if (link){
			sp_track *track = sp_link_as_track(link);
			if (track){
				new_tracks[j++] = track;
				printf("%s\n", sp_track_name(track));
			}
		}
	}
	fclose(fp);
	fflush(stderr);
	if (j < 1){
		return;
	}
	fprintf(stdout, "Adding %i tracks \n", j);
	sp_error myerror = sp_playlist_add_tracks(pl, new_tracks, j, 0);

	if (myerror != SP_ERROR_OK) {
		fprintf(stderr, "Error adding track with error %d", myerror);
	}
	sp_playlist_add_callbacks(pl, &sp_callbacks, "yay");
	fprintf(stderr, "Waiting for Spotify to catch up...");
    while (sp_playlist_has_pending_changes(pl)){
		printf(".");
	}
    fprintf(stderr, "       [DONE]\n");
}

void metadata_updated(sp_session *session)
{
	if (update_running == 0) {
		update_playlists(session);
	} else {
		usleep(100000);
		g_exit_code = 1;
	}
}



/**
 * Callback called when the session has been terminated.
 */
void session_terminated(void)
{
}

/**
 * This callback is called from an internal libspotify thread to ask us to
 * reiterate the main loop.
 *
 * The most straight forward way to do this is using Unix signals. We use
 * SIGIO. signal(7) in Linux says "I/O now possible" which sounds reasonable.
 *
 * @sa sp_session_callbacks#notify_main_thread
 */
static void notify_main_thread(sp_session *session)
{
	pthread_kill(g_main_thread, SIGIO);
}

/**
 * This callback is called for log messages.
 *
 * @sa sp_session_callbacks#log_message
 */


/**
 * The structure containing pointers to the above callbacks.
 *
 * Any member may be NULL to ignore that event.
 *
 * @sa sp_session_callbacks
 */
static sp_session_callbacks g_callbacks = {
	&logged_in,
	&logged_out,
	&metadata_updated,
	&connection_error,
	NULL,
	&notify_main_thread,
	NULL,
	NULL,
	&log_message
};



static void loop(sp_session *session)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGIO);

	while (g_exit_code < 0) {
		int timeout = -1;

		pthread_sigmask(SIG_BLOCK, &sigset, NULL);
		sp_session_process_events(session, &timeout);
		pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
		usleep(timeout * 1000);

	}
		

}
static void sigIgn(int signo)
{
}


int main(int argc, char **argv)
{
	sp_session_config config;
	sp_error error;
	sp_session *session;

	// Sending passwords on the command line is bad in general.
	// We do it here for brevity.
	if (argc < 3 || argv[1][0] == '-') {
		fprintf(stderr, "usage: %s <username> <password>\n",
		                basename(argv[0]));
		return 1;
	}

	// Setup for waking up the main thread in notify_main_thread()
	g_main_thread = pthread_self();
	signal(SIGIO, &sigIgn);

	// Always do this. It allows libspotify to check for
	// header/library inconsistencies.
	config.api_version = SPOTIFY_API_VERSION;

	// The path of the directory to store the cache. This must be specified.
	// Please read the documentation on preferred values.
	config.cache_location = "tmp";

	// The path of the directory to store the settings. This must be specified.
	// Please read the documentation on preferred values.
	config.settings_location = "tmp";

	// The key of the application. They are generated by Spotify,
	// and are specific to each application using libspotify.
	config.application_key = g_appkey;
	config.application_key_size = g_appkey_size;

	// This identifies the application using some
	// free-text string [1, 255] characters.
	config.user_agent = "spotify-session-example";

	// Register the callbacks.
	config.callbacks = &g_callbacks;

	error = sp_session_init(&config, &session);

	if (SP_ERROR_OK != error) {
		fprintf(stderr, "failed to create session: %s\n",
		                sp_error_message(error));
		return 2;
	}

	// Login using the credentials given on the command line.
	error = sp_session_login(session, argv[1], argv[2]);

	if (SP_ERROR_OK != error) {
		fprintf(stderr, "failed to login: %s\n",
		                sp_error_message(error));
		return 3;
	}

	loop(session);
	session_terminated();

	return 0;
}
