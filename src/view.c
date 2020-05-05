#include "view.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <locale.h>
#include <ncursesw/ncurses.h>
#include <stdio.h>

#include "buffer.h"
#include "mandown.h"

void indentHandler(struct parts *dest, xmlChar *content, int indentChar)
{
  int i, length;

  length = xmlStrlen(content);
  // wprintw(dest->container, "%d", indentChar);
  getyx(dest->container, dest->curY, dest->curX);

  for (i = 0; i < length; i++) {
    getyx(dest->container, dest->curY, dest->curX);

    if ((dest->curX >= dest->width - 1) || content[i] == '\n') {
      if (dest->curY >= dest->height - 1) {
        dest->height += 1;
        wresize(dest->container, dest->height, dest->width);
      }
      wprintw(dest->container, "\0%c", indentChar);
    }
    else if (dest->curX == 0) {
      wprintw(dest->container, "%c", indentChar);
    }
    // if (content[i] == '\n' && !formated) {
    //   content[i] = ' ';
    // }
    waddch(dest->container, content[i]);
  }
}

/* nodeHandler: set rendering rule for node  */
void nodeHandler(xmlNode *node, struct parts *dest)
{
  xmlNode *curNode;
  int indentChar;

  for (curNode = node; curNode != NULL; curNode = curNode->next) {
    if (curNode->type == XML_ELEMENT_NODE) {
      if (xmlStrEqual((xmlChar *)"h1", curNode->name)) {
        wprintw(dest->container, "README(7)\n\nNAME\n\t");
      }
      else if (xmlStrEqual((xmlChar *)"h2", curNode->name)) {
        waddch(dest->container, '\n');
      }
      else if (xmlStrEqual((xmlChar *)"h3", curNode->name)) {
        wprintw(dest->container, "\n%*c", 3, ' ');
      }
      else if (xmlStrEqual((xmlChar *)"h4", curNode->name)) {
        wprintw(dest->container, "\n%*cSECTION: ", 6, ' ');
      }
      else if (xmlStrEqual((xmlChar *)"h5", curNode->name)) {
        wprintw(dest->container, "\n%*cSUB SECTION: ", 9, ' ');
      }
      else if (xmlStrEqual((xmlChar *)"h6", curNode->name)) {
        wprintw(dest->container, "\n%*cPOINT: ", 12, ' ');
      }
    }
    else if (curNode->type == XML_TEXT_NODE) {
      indentChar = 0;

      if (xmlStrEqual((xmlChar *)"p", curNode->parent->name)) {
        indentChar = '\t';
      }
      else if (xmlStrEqual((xmlChar *)"code", curNode->parent->name)) {
        indentChar = '\t';
      }
      else if (xmlStrEqual((xmlChar *)"ol", curNode->parent->name)) {
      }
      else if (xmlStrEqual((xmlChar *)"ul", curNode->parent->name)) {
      }
      else if (xmlStrEqual((xmlChar *)"li", curNode->parent->name)) {
        indentChar = '\t';
      }
      else if (xmlStrEqual((xmlChar *)"strong", curNode->parent->name)) {
      }
      else if (xmlStrEqual((xmlChar *)"em", curNode->parent->name)) {
      }
      // if (xmlStrlen(curNode->content) > 1)

      indentHandler(dest, curNode->content, indentChar);
      wattrset(dest->container, A_NORMAL);
      // waddch(dest->container, '\n');
    }
    nodeHandler(curNode->children, dest);
  }
}

/* partsNew: allocation of a new pad */
struct parts *
partsNew()
{
  struct parts *ret;
  ret = malloc(sizeof(struct parts));

  if (ret) {
    ret->container = NULL;
    ret->height = 0;
    ret->width = 0;
    ret->curY = 0;
    ret->curX = 0;
  }
  return ret;
}

void partsFree(struct parts *part)
{
  if (!part)
    return;

  delwin(part->container);
  free(part);
}

int view(struct buf *ob, int blocks)
{
  int ymax, xmax, key, curLine;
  struct parts *content, *info;
  xmlDoc *doc;
  xmlNode *rootNode;

  LIBXML_TEST_VERSION;

  doc = xmlReadMemory((char *)(ob->data), (int)(ob->size), "noname.xml", NULL, 0);
  if (doc == NULL) {
    error("Failed to parse document\n");
    return 1;
  }

  rootNode = xmlDocGetRootElement(doc);
  if (rootNode == NULL) {
    error("empty document\n");
    xmlFreeDoc(doc);
    return 1;
  }

  /* Initialize ncurses */
  setlocale(LC_CTYPE, "");
  initscr();
  // keypad(stdscr, TRUE); /* enable arrow keys */
  curs_set(0); /* disable cursor */
  cbreak();    /* make getch() process one char at a time */
  noecho();    /* disable output of keyboard typing */
  // idlok(stdscr, TRUE);  /* allow use of insert/delete line */

  /* Initialize all colors if terminal support color */
  // if (has_colors()) {
  //   start_color();
  //   use_default_colors();
  //   init_pair(BLK, COLOR_RED, -1);
  //   init_pair(RED, COLOR_RED, -1);
  //   init_pair(GRN, COLOR_GREEN, -1);
  //   init_pair(YEL, COLOR_YELLOW, -1);
  //   init_pair(BLU, COLOR_BLUE, -1);
  //   init_pair(MAG, COLOR_MAGENTA, -1);
  //   init_pair(CYN, COLOR_CYAN, -1);
  //   init_pair(WHT, COLOR_WHITE, -1);
  // }

  getmaxyx(stdscr, ymax, xmax);
  content = partsNew();
  content->height = blocks;
  content->width = xmax;
  content->container = newpad(content->height, content->width);
  keypad(content->container, TRUE); /* enable arrow keys */

  info = partsNew();
  info->height = 1;
  info->width = xmax;
  info->container = newwin(info->height, info->width, ymax - 1, 0);
  scrollok(info->container, TRUE);

  /* Render the result */
  nodeHandler(xmlFirstElementChild(rootNode), content);

  curLine = 0;
  prefresh(content->container, curLine, 0, 0, 0, ymax - 1, xmax);
  wprintw(info->container, "\n%d%% (press q to quit)", ((curLine + ymax + 1) * 100) / content->height);
  wrefresh(info->container);
  // wgetch(content->container);

  while ((key = wgetch(content->container)) != 'q') {
    switch (key) {
      case KEY_UP:
        if (curLine > 0)
          curLine--;
        break;
      case KEY_DOWN:
        if (curLine + ymax < content->height - 1)
          curLine++;
        break;
      default:
        break;
    }
    prefresh(content->container, curLine, 0, 0, 0, ymax - 1, xmax);
    wprintw(info->container, "\n%d%% (press q to quit)", ((curLine + ymax + 1) * 100) / content->height);
    wrefresh(info->container);
  }

  /* Clean up */
  xmlFreeDoc(doc);
  partsFree(content);
  partsFree(info);
  endwin();
  return 0;
}

// int newlines = 0, Choice = 0, Key = 0;
// for (int i = 0; i < ob->size; i++)
//     if (ob->data[i] == '\n') newlines++;
// int PadHeight = ((ob->size - newlines) / Width + newlines + 1);
// mypad = newpad(PadHeight, Width);
// keypad(content->container, true);
// waddwstr(content->container, c_str());
// refresh();
// int curLine = 0;

/* vim: set filetype=c: */
