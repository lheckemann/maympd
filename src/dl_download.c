/* ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <errno.h>
#include <mpd/client.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <mpd/directory.h>
#include <mpd/recv.h>


int download_stream(char *p_charbuf, char *dir, char **song)
{

	int pipefd[2];
	char res[1048576];

	if (pipe(pipefd) == -1){
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();
	if (pid == 0) /*child */
	{
		close(pipefd[0]);  /* close unused read end */
		dup2(pipefd[1], STDOUT_FILENO);

		if (chdir(dir)) {
			fprintf(stderr, "chdir %s: %s", dir, strerror(errno));
			_exit(EXIT_FAILURE);
		}

		char *cmd[] = {
			"yt-dlp",
			"-f", "bestaudio[ext!=aac]/bestaudio",
			"--extract-audio",
			"--audio-format=best",
			"-o", "%(title)s-%(id)s.%(ext)s",
			"--add-metadata",
			"--newline",
			"--no-mtime",
			"--",
			p_charbuf,
			NULL
		};
		execvp(cmd[0], cmd);
		_exit(EXIT_FAILURE);

	}
	else
	{
		close(pipefd[1]); /* close write end */
		waitpid(pid, NULL, 0);
		read(pipefd[0], res, sizeof(res));
		if(strstr(res, "100%") != NULL)
		{
		  char *FFMPEG_OUT = "[Metadata] Adding metadata to \"";
		  char *path = malloc(8192);
		  char *fp, *token, *saveptr;

		  for(fp = res; ; fp = NULL) {
			token = strtok_r(fp, "\n", &saveptr);
			if(token == NULL) break;
			fprintf(stderr, "%s\n", token);
			fp = strstr(token, FFMPEG_OUT);
			if(fp != NULL) {
			  snprintf(path, 8192, "%s", fp+strlen(FFMPEG_OUT));
			  path[strlen(path)-1] = '\0';
			};
		  };

		  fprintf(stderr, "Downloaded: %s\n", path);
		  *song = path;
			return 0;
		}
		else
		{
			fprintf(stderr, "Error while downloading :\n %s\n", p_charbuf);
			return 1;
		}

	}

}
