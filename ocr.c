/*
 * OCR
 *
 * By mmr <mmr@b1n.org>
 *
 * $Id: ocr.c,v 1.21 2006/01/12 22:32:38 mmr Exp $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <err.h>


#define SUCESSO     0
#define FALHA       1

/*
 * Uma imagem eh formada de (Largura X Altura X Cores)
 *
 * A imagem que queremos, tem 250x100 Pixels à  256 cores
 * 256 cores cabem em um 1byte.
 * Portanto, 250x100x1 ou 250*100*1
 * Isso deve caber a imagem inteira. 
 *
 */

#define BUFGTAM         250*100*1   /* Buffer Grande  */
#define BUFMTAM         BUFSIZ*8    /* Buffer Medio   */
#define BUFPTAM         BUFSIZ      /* Buffer Pequeno */ 
#define HEADER_IMAGEM   "Content-Type: image/pjpeg\r\n\r\n"

#define PORTA       80
#define URL         "www.receita.fazenda.gov.br"
#define URL_GET     "/Scripts/srf/img/srfimg.dll"
#define USER_AGENT  "Mozilla/4.0 (compatible; MSIE 6.0; Windows 98; Win 9x 4.90)"

int criaSocket(void);
int conecta(int);
char *trataDados(char *);
char *pegaDados(int, int);

int
main(void)
{
    FILE *tst;
    int sock;
    int conexao;
    int ret;
    char *dados;

    /* Cria Socket */
    sock = criaSocket();

    /* Conecta-se ao Host Remoto */
    conexao = conecta(sock);

    /* Faz Requisicao da URL_GET descrita, retornando os Dados */
    dados = pegaDados(sock, conexao);

    /* Ve se recebeu Dados */
    if(dados != NULL)
    {
        /* Trata Dados (tira headers) */
        dados = trataDados(dados);
        if(dados != NULL)
        {
            // printf("Dados tratados com sucesso.\n"); 
            // printf("%s\n", dados);
            tst = fopen("/var/www/htdocs/b1n/teste.jpg", "w");
            fprintf(tst, "%s", dados);
            fclose(tst);

            ret = SUCESSO;
        }
        else
        {
            (void) fprintf(stderr, "Erro ao tratar dados.\n");
            ret = FALHA;
        }
    }
    else
    {
        (void) fprintf(stderr, "Nenhum dado foi pego.\n");    
        ret = FALHA;
    }

    /* Liberando Memoria e Fechando File Descriptors */
    //free(dados);
    (void) close(sock);

    return ret;
}

int
criaSocket(void)
{
    return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

int
conecta(int sock)
{
    int ret;
    struct hostent *host;
    struct sockaddr_in servidor;

    if(sock)
    {
        /* Pegando IP do Host Remoto */
        host = gethostbyname(URL); 

        if(host != NULL)
        {
            /* Limpando a Struct */
            (void) memset(&servidor, 0, sizeof(servidor));

            /* Colocando dados para Conexao na Struct */
            servidor.sin_family = host->h_addrtype;
            servidor.sin_port   = htons(PORTA);
            (void) memcpy(&servidor.sin_addr, host->h_addr_list[0], sizeof(servidor.sin_addr));

            /* Conectando ao Host Remoto */
            ret = connect(sock, (struct sockaddr *)&servidor, sizeof(servidor));
        }
        else
        {
            /* Erro ao tentar resolver o Nome do Host Remoto */
            errx(FALHA, "gethostbyname: %s", hstrerror(h_errno));
            ret = FALHA;
        }
    }
    else
    {
        /* Erro na Criacao do Socket */
        err(FALHA, "socket");
        ret = FALHA;
    }

    return ret;
}

char *
pegaDados(int sock, int conexao)
{
    char buf[BUFMTAM];
    char *requisicao;
    char *ret;

    ret = (char *) malloc(BUFGTAM);
    bzero(ret, BUFGTAM);

    requisicao = (char *) malloc(BUFPTAM);

    if(conexao == 0)
    {
        /* Faz Requisicao HTTP */
        (void)
        snprintf(
            requisicao,
            BUFPTAM,
            "GET %s HTTP/1.0\r\n"
            "User-Agent: %s\r\n"
            "Host: %s\r\n"
            "Accept: */*\r\n"
            "Connection: Keep-Alive\r\n"
            URL_GET, USER_AGENT, URL);

        (void) send(sock, requisicao, strlen(requisicao), 0);

        /* Le Resposta do WebServer e Concatena em Ret */
        while(recv(sock, buf, BUFMTAM, 0) > 0)
        {
            if(strlcat(ret, buf, BUFGTAM) >= BUFGTAM)
            {
                err(FALHA, "Espaco insuficiente para comportar dados. (%s)\n", BUFGTAM);
            }

        }
    }
    else
    {
        errx(FALHA, "connect: %s", strerror(errno));
    }

    free(requisicao);
    return ret;
}

char *
trataDados(char *dados)
{
    char *ret;

    ret = strstr(dados, HEADER_IMAGEM);

    if(ret != NULL)
    {
        ret += strlen(HEADER_IMAGEM);
    }
    else
    {
        fprintf(stderr, "trataDados: Erro ao tratar dados\n");
    }

    return ret;
}
