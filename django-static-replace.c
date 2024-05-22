#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* filePath;
    FILE* file;
} FileInfos;

/* Substring d'une chaine avec bornes inclus */
char* substr(const char* chaine, int from, int to){
    int len = strlen(chaine);
    if(from < 0 || to < 0 || to >= len || from > to){
        printf("Erreur lors de l'appel de substr sur \"%s\"\nAvec from=%d & to=%d\n", chaine, from, to);
        return NULL;
    }

    char* buff = malloc(sizeof(char) * (to - from + 2));
    if(buff == NULL){
        printf("Erreur lors de l'allocation memoire\n");
        exit(1);
    }

    for (int i = from; i <= to; i++){
        buff[i - from] = chaine[i];
    }
    buff[to - from + 1] = '\0';
    return buff;
}

// Vérifie si une chaine entre quotes est un lien
int isLink(const char* chaine){
    if(strlen(chaine) < 8){
        return 0;
    }

    char* chaine1 = substr(chaine, 0, 7);
    char* chaine2 = substr(chaine, 0, 8);
    int ret = (!strcmp(chaine1, "\"http://") || !strcmp(chaine2, "\"https://"));

    free(chaine1);
    free(chaine2);
    return ret;
}

// Vérifie si une chaine entre quotes est un lien interne HTML
int is_html(const char* line){
    int len = strlen(line) - 1;
    if(len < 8){ // car strlen("\"x.html\"") = 8
        return 0;
    }

    char* ext = substr(line, len - 5, len - 1);
    int ret = (strcmp(ext, ".html") == 0);

    free(ext);
    return ret;
}

void process(FileInfos fileinfo){
    int status = 1, chars = 0, i=0, start, end, curs=0;
    char *s = NULL; char c;

    while((c = fgetc(fileinfo.file)) != EOF){
        s = realloc(s, sizeof(char)*(chars+1));
        if(s == NULL){
            printf("Erreur lors de l allocation memoire\n");
            exit(1);
        }
        s[chars] = c;
        chars ++;
    } 

    s = realloc(s, sizeof(char)*(chars+1)); 
    if(s == NULL){
        printf("Erreur lors de l allocation memoire\n");
        exit(1);
    }
    s[chars] = '\0';

    char *copie = malloc(chars * 2 + 1);
    if(copie == NULL){
        free(s);
        printf("Erreur lors de l'allocation memoire\n");
        exit(1);
    }

    while (i < chars){
        int copiecote = 0;
        if(s[i] == '"'){
            if(i - 5 < 0){
                i++;
                continue;
            }

            char* href = substr(s, i - 5, i);
            char* src = substr(s, i - 4, i);

            if(!strcmp(href, "href=\"") || !strcmp(src, "src=\"")){
                start = i;
                status *= -1;
            } else if(status == -1){
                end = i;
                char* line = substr(s, start, end);
                if(!isLink(line) && !is_html(line) && !(strlen(line)==3 && line[1]=='#') && strlen(line)!=2){
                    char debut[20]; strcpy(debut, "\"{% static "); 
                    char fin[20]; strcpy(fin, " %}\"");
                    int sauts = 0;
                    for(int ind=0; ind<11; ind++){
                        copie[curs+ind] = debut[ind];
                        sauts++;
                    }
                    curs += sauts; sauts = 0;
                    for(int ind=start; ind<=end; ind++){
                        copie[curs+ind-start] = line[ind-start];
                        sauts++;
                    }
                    curs += sauts; sauts = 0;
                    for(int ind=0; ind<4; ind++){
                        copie[curs+ind] = fin[ind];
                        sauts++;
                    }
                    curs += sauts; 
                } else {
                    int sauts = 0;
                    for(int ind=start; ind<=end; ind++){
                        copie[curs+ind-start] = line[ind-start];
                        sauts++;
                    }
                    curs += sauts;
                }
                free(line);
                status *= -1;
            } else {
                copiecote = 1;
            }

            free(href);
            free(src);

        } else if(status == 1){
            copie[curs++] = s[i];
        }
        if(copiecote){
            copie[curs++] = s[i];
        }
        i++;
    }
    copie[curs] = '\0';

    free(s);

    char* filenameToStore = malloc(strlen(fileinfo.filePath)+1+4);
    if(filenameToStore == NULL){
        exit(1);
    }
    strcpy(filenameToStore, fileinfo.filePath);
    strcat(filenameToStore, "-mhd");

    FILE* output = fopen(filenameToStore, "w");
    if(output == NULL){
        exit(1);
    }
    fprintf(output, "%s", copie);
    printf("<   Output : %s\n", filenameToStore);
    free(copie);
    fclose(output);
}

int main(int argc, char* argv[]){
    if(argc < 2){
        printf("Erreur : Arguments insuffisants !!!\n");
        printf("Usage : %s PathToFilename1 [PathToOtherFiles]\n", argv[0]);
        exit(1);
    }

    int saisie;

    printf("Dalal Ak Jammeu !\n");
    printf("Pour éviter des erreurs, veuillez vous assurer qu'il n'y a pas d'espaces à gauche et à droite de '=' lorsque vous utilisez href ou src\n\n");
    printf("Entrez 1 pour continuer >>> ");

    scanf("%d", &saisie);
    if(saisie != 1){
        printf("Fermeture du programme ...\n");
        exit(1);
    }

    FileInfos* files = (FileInfos*)malloc((argc - 1) * sizeof(FileInfos));
    if(files == NULL){
        printf("Erreur lors de l'allocation memoire\n");
        exit(1);
    }

    for (int i = 1; i < argc; i++){
        files[i - 1].file = fopen(argv[i], "r");
        if(files[i - 1].file == NULL){
            printf("Erreur : Le fichier \"%s\" est introuvable\n", argv[i]);
            free(files);
            exit(1);
        }
        files[i - 1].filePath = argv[i];
    }

    for (int i = 0; i < argc - 1; i++){
        printf("\nProcessing [%d/%d] : \n>   Input : %s\n", i + 1, argc - 1, files[i].filePath);
        process(files[i]);
        fclose(files[i].file);
    }

    free(files);
    printf("\n\nLe traitement est termine !");
    return 0;
}
