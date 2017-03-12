#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

/*I hope it is close enough.*/
#define MAXSIZE 2048

/*To temporary hold file and directory names in alphabetical order.*/
struct List {
	char *name;
	struct List *next;
};

/*All nodes of the binary search tree are actually a pointer of BSTree*/
struct BSTree {
	char *word;
	int count;
	struct FileLog *hLogs; /*Ordered linked list head of file logs, one head per word*/
	struct BSTree *left;
	struct BSTree *right;
};

/*File name and count info for files those contains the word. In ordered linked list format.*/
struct FileLog {
	char *fname;
	int count;
	struct FileLog *next;
};

/*Below 4 must-have binary search functions had been initialized.*/
struct BSTree* insertToTree(struct BSTree *node, char* word, char* fname);
struct BSTree *searchInTree(struct BSTree *node, char *str, int depth,
		FILE *out);
struct BSTree *findMinInTree(struct BSTree *node);
struct BSTree *removeFromTree(struct BSTree *node, char* str);
/*Tree will recursively build by iterating dirs in this function*/
void buildTheTree(struct BSTree **words, char *dirname);
/*Files and dirs are inserted into an ordered linked list to order them easily.*/
void insertToList(struct List **phead, struct List **node);
/*Answer of how many times and which file a word exists is stored into a FileLog header for per node.*/
void insertToFileLog(struct FileLog **phead, char *fname);
/*While building tree, just before traversing other dirs,
 * scanned files processed and their words will be inserted to the tree.*/
void processFile(struct BSTree **head, char *file);
/*Below 3 function prints the tree in 3 ways.*/
void printTree(struct BSTree *node, int depth, FILE *out);
void printTreeASC(struct BSTree *node, int depth, FILE *out);
void printTreeDSC(struct BSTree *node, int depth, FILE *out);
/*printw just prints a node.*/
void printw(struct BSTree *node, int depth, FILE *out);
/*These are just helper functions. isDir returns 0 if its a file,
 * toLowerCase returns a new allocated strings address in lowercase of its input.*/
int isDir(char *path);
char *toLowerCase(char *str);

int main(int argc, char *argv[]) {

	if (argc != 3) {
		fprintf(stderr, "Wrong arguments!");
		return 1;
	}
	/*1--Building the tree:*/
	struct BSTree *root = NULL;
	buildTheTree(&root, argv[1]);

	/*2--Preparing for i/o operations:*/
	FILE *in, *out;
	in = fopen(argv[2], "r");
	out = fopen("output.txt", "w+");

	/*3--Reading commands and output results:*/
	char line[MAXSIZE], *token;
	while (fgets(line, sizeof(line), in)) {
		/*First token is command, one of below ifs will execute.*/
		/*To parse line \r character will be necessary if input.txt produced in windows,
		 *because return carriage in linux&unix=\n but in windows=\r\n.*/
		char *linecopy = (char *) malloc(sizeof(char) * (strlen(line) + 1));
		strcpy(linecopy, line);
		/*if there is a newline at end of line, it should be deleted since we will already adding one.*/
		if (linecopy[strlen(linecopy) - 1] == '\n')
			linecopy[strlen(linecopy) - 1] = '\0';

		token = strtok(line, " \r\n");
		if (strcmp(token, "PRINT") == 0) {
			printf("%s\n", linecopy);
			fprintf(out, "%s\n", linecopy);
			token = strtok('\0', " \r\n");
			if (strcmp(token, "TREE") == 0) {
				token = strtok('\0', " \r\n");
				if (strcmp(token, "ASC") == 0) {
					printTreeASC(root, 1, out);
				} else if (strcmp(token, "DSC") == 0) {
					printTreeDSC(root, 1, out);
				} else {
					printTree(root, 1, out);
				}
			}
		}

		else if (strcmp(token, "SEARCH") == 0) {
			printf("%s\n", token);
			fprintf(out, "%s\n", token);
			token = strtok('\0', " \r\n");
			searchInTree(root, toLowerCase(token), 1, out);
		}

		else if (strcmp(token, "ADD") == 0) {
			printf("%s", token);
			fprintf(out, "%s", token);
			token = strtok('\0', " \r\n");
			processFile(&root, token);
			printf(" %s\n", token);
			fprintf(out, " %s\n", token);

		} else if (strcmp(token, "REMOVE") == 0) {
			printf("%s", token);
			fprintf(out, "%s", token);
			token = strtok('\0', " \r\n");
			if (removeFromTree(root, token)) {
				printf(" %s\n", token);
				fprintf(out, " %s\n", token);
			} else {
				printf("\n");
				fprintf(out, "\n");
			}
		}
	}
	return 0;
}

void buildTheTree(struct BSTree **hTree, char *dirname) {
	chdir(dirname);

	DIR *dir = opendir(".");
	struct dirent *pdir;

	/*hFile and hDir are head of ordered linked lists for scanned files and dirs.*/
	struct List *hFile = NULL;
	struct List *hDir = NULL;

	char *fname;
	/*Scan and insert files&directories into ordered linked lists*/
	while ((pdir = readdir(dir)) != NULL) {
		fname = pdir->d_name;
		if (!strcmp(fname, ".") || !strcmp(fname, ".."))
			continue;
		if (isDir(fname)) {
			struct List *newDir = (struct List*) malloc(sizeof(struct List));
			newDir->name = (char *) malloc((strlen(fname) + 1) * sizeof(char));
			strcpy(newDir->name, fname);
			insertToList(&hDir, &newDir);
		} else {
			struct List *newFile = (struct List*) malloc(sizeof(struct List));
			newFile->name = (char *) malloc((strlen(fname) + 1) * sizeof(char));
			strcpy(newFile->name, fname);
			insertToList(&hFile, &newFile);
		}

	}
	/*Add files to tree*/
	struct List *temp;
	while (hFile) {
		processFile(hTree, hFile->name);
		temp = hFile;
		hFile = hFile->next;
		free(temp);
	}
	/*Iterate among dirs recursively*/
	while (hDir) {
		buildTheTree(hTree, hDir->name);
		temp = hDir;
		hDir = hDir->next;
		free(temp);
	}
	chdir("..");
	/*Free both lists*/

}

void processFile(struct BSTree **hTree, char *file) {
	FILE *fp;
	fp = fopen(file, "r");
	char buf[MAXSIZE], *string, *fname, *temp;
	int i, j, len = strlen(file);
	for (i = len - 2; i > 0; i--) {
		if (i == 1) {
			fname = file;
			break;
		} else if (file[i] == '/') {
			fname = (char *) malloc(sizeof(char) * (len - 1 - i));
			for (j = 0, i++; j < len; j++, i++) {
				fname[j] = file[i];
			}
			break;
		}
	}
	while (fscanf(fp, "%2047s", buf) != EOF) {
		len = strlen(buf);
		string = (char *) malloc((len + 1) * sizeof(char));
		for (i = 0, j = 0; i < len; i++) {
			if (((int) buf[i] > 96 && (int) buf[i] < 123) || (int) buf[i] == 45
					|| ((int) buf[i] > 47 && (int) buf[i] < 58)) {
				string[j++] = buf[i];
			} else if ((int) buf[i] > 64 && (int) buf[i] < 91) {
				string[j++] = ((int) buf[i]) + 32;
			}
		}
		string[j] = '\0';
		*hTree = insertToTree(*hTree, string, fname);
	}
	fclose(fp);
}

struct BSTree *insertToTree(struct BSTree *node, char* newWord, char* fname) {
	if (node == NULL) {
		struct BSTree *newNode = (struct BSTree *) malloc(
				sizeof(struct BSTree));
		newNode->word = (char*) malloc((strlen(newWord) + 1) * sizeof(char));
		strcpy(newNode->word, newWord);
		newNode->count = 1;

		/*Since its first time, hLogs(ordered linked list head of file logs, per word) initializing;*/
		struct FileLog *log = (struct FileLog *) malloc(sizeof(struct FileLog));
		log->fname = (char *) malloc(sizeof(char) * (strlen(fname) + 1));
		strcpy(log->fname, fname);
		log->count = 1;
		log->next = NULL;
		newNode->hLogs = log;

		newNode->left = NULL;
		newNode->right = NULL;
		return newNode;
	}
	if (strcmp(newWord, node->word) > 0) {
		node->right = insertToTree(node->right, newWord, fname);
	} else if (strcmp(newWord, node->word) < 0) {
		node->left = insertToTree(node->left, newWord, fname);
	} else {
		/*this else block means data is already in tree, so we need to increment the count.*/
		/*HARD: LOG FILENAMES BY COUNT.*/
		/*printf("%d. time of %s\n", ++(node->count), node->word);*/
		++(node->count);
		insertToFileLog(&(node->hLogs), fname);
	}

	return node;

}

struct BSTree *searchInTree(struct BSTree *node, char *str, int depth,
		FILE *out) {
	if (node == NULL) {
		return NULL;
	}
	if (strcmp(str, node->word) > 0) {
		return searchInTree(node->right, str, depth + 1, out);
	} else if (strcmp(str, node->word) < 0) {
		return searchInTree(node->left, str, depth + 1, out);
	} else {
		/*Found!*/
		printw(node, depth, out);
		return node;
	}
}

struct BSTree *removeFromTree(struct BSTree *node, char* str) {
	struct BSTree *tmp;
	if (node == NULL) {
		/*element not found, nothing to do.*/
		return NULL;
	} else if (strcmp(str, node->word) > 0) {
		node->right = removeFromTree(node->right, str);
	} else if (strcmp(str, node->word) < 0) {
		node->left = removeFromTree(node->left, str);
	} else {
		if (node->left && node->right) {
			tmp = findMinInTree(node->right);
			node->word = (char *) realloc(node->word,(strlen(tmp->word) + 1) * sizeof(char));
			strcpy(node->word, tmp->word);
			node->count = tmp->count;
			node->hLogs = tmp->hLogs;
			node->right = removeFromTree(node->right, tmp->word);
		} else {
			if (node->right)
				node = node->right;
			else if (node->left)
				node = node->left;
		}
	}
	return node;

}

struct BSTree *findMinInTree(struct BSTree *node) {
	if (!node) {
		return NULL;
	}
	if (!node->left)
		return node;
	else
		return findMinInTree(node->left);
}
void printw(struct BSTree *node, int depth, FILE *out) {
	printf(" %s\n\t%d\n\t%d\n", node->word, node->count, depth);
	fprintf(out, " %s\n\t%d\n\t%d\n", node->word, node->count, depth);
	struct FileLog *log = node->hLogs;
	while (log) {
		printf("\t%s %d\n", log->fname, log->count);
		fprintf(out, "\t%s %d\n", log->fname, log->count);
		log = log->next;
	}
}

void printTree(struct BSTree *node, int depth, FILE *out) {
	if (node == NULL)
		return;
	printw(node, depth, out);
	printTree(node->left, depth + 1, out);
	printTree(node->right, depth + 1, out);
}

void printTreeASC(struct BSTree *node, int depth, FILE *out) {
	if (node == NULL)
		return;
	printTreeASC(node->left, depth + 1, out);
	printw(node, depth, out);
	printTreeASC(node->right, depth + 1, out);
}

void printTreeDSC(struct BSTree *node, int depth, FILE *out) {
	if (node == NULL)
		return;
	printTreeDSC(node->right, depth + 1, out);
	printw(node, depth, out);
	printTreeDSC(node->left, depth + 1, out);
}

void insertToFileLog(struct FileLog **phead, char *fname) {
	struct FileLog *head = *phead;
	/*if its first time, create it and add it to head.*/
	if (head == NULL) {
		struct FileLog *log = (struct FileLog *) malloc(sizeof(struct FileLog));
		log->fname = (char *) malloc(sizeof(char) * strlen(fname));
		strcpy(log->fname, fname);
		log->count = 1;
		log->next = NULL;
		head = log;
	} else {
		struct FileLog *temp = head, *prev;
		while (strcmp(temp->fname, fname) < 0) {
			prev = temp;
			temp = temp->next;
			if (temp == NULL)
				break;
		}
		if (temp != NULL) {
			if (strcmp(temp->fname, fname) == 0) {
				/*found existing filename, so just increment it's count.*/
				++(temp->count);
			}
		} else {
			struct FileLog *log = (struct FileLog *) malloc(
					sizeof(struct FileLog));
			log->fname = (char *) malloc(sizeof(char) * strlen(fname));
			strcpy(log->fname, fname);
			log->count = 1;
			/*now insert just created log by fname inreasing order.*/
			if (temp == head) {
				log->next = head;
				head = log;
			} else if (temp == NULL) {
				log->next = NULL;
				prev->next = log;
			} else {
				prev->next = log;
				log->next = temp;
			}
		}
	}

	*phead = head;
}

void insertToList(struct List **phead, struct List **node) {
	struct List *head = *phead;
	struct List *curr = *node;
	if (head == NULL) {
		curr->next = NULL;
		head = curr;
	} else {
		struct List *temp = head, *prev;
		while (strcmp(temp->name, curr->name) < 0) {
			prev = temp;
			temp = temp->next;
			if (temp == NULL)
				break;
		}
		if (temp == head) {
			curr->next = head;
			head = curr;
		} else if (temp == NULL) {
			curr->next = NULL;
			prev->next = curr;
		} else {
			prev->next = curr;
			curr->next = temp;
		}
	}
	*phead = head;
	*node = curr;
}

int isDir(char *fname) {
	char path[256];
	sprintf(path, "./%s", fname);
	struct stat buf;
	if (stat(fname, &buf)) {
		return 0;
	}
	return S_ISDIR(buf.st_mode);
}

char *toLowerCase(char *str) {
	char *newstr;
	int i, j, len;
	len = strlen(str);
	newstr = (char *) malloc((len + 1) * sizeof(char));
	for (i = 0, j = 0; i < len; i++) {
		if ((int) str[i] > 64 && (int) str[i] < 91) {
			newstr[j++] = ((int) str[i]) + 32;
		} else {
			newstr[j++] = str[i];
		}
	}
	newstr[j] = '\0';
	return newstr;
}
