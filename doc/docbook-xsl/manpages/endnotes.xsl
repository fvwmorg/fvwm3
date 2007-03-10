<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:exsl="http://exslt.org/common"
                xmlns:ng="http://docbook.org/docbook-ng"
                xmlns:db="http://docbook.org/ns/docbook"
                xmlns:xlink="http://www.w3.org/1999/xlink"
                exclude-result-prefixes="db ng exsl xlink"
                version='1.0'>

<!-- ********************************************************************
     $Id$
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://docbook.sf.net/release/xsl/current/ for
     copyright and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->
<!-- * -->
<!-- * The templates in this file handle elements whose contents can't -->
<!-- * be displayed completely within the main text flow in output, but -->
<!-- * instead need to be displayed "out of line". Those elements are: -->
<!-- * -->
<!-- *   - elements providing annotative text (annotation|alt|footnote) -->
<!-- *   - elements pointing at external resources (ulink, link, and -->
<!-- *     any elements with xlink:href attributes; and imagedata, -->
<!-- *     audiodata, and videodata - which (using their fileref -->
<!-- *     attribute) reference external files -->
<!-- * -->
<!-- * Within this stylesheet, the above are collectively referred to as -->
<!-- * a "notesources". This stylesheet handles those notesources in -->
<!-- * this way: -->
<!-- * -->
<!-- * 1. Constructs a numbered in-memory index of all unique "earmarks“ -->
<!-- *    of all notesources in the document. For each link, the -->
<!-- *    earmark is the value of its url or xlink:href attribute; for -->
<!-- *    each imagedata|audiodata|videodata: the value of its fileref -->
<!-- *    attribute; for each annotative element: its content. -->
<!-- * -->
<!-- *    Notesources with the same earmark are assigned the same -->
<!-- *    number. -->
<!-- * -->
<!-- *    By design, that index excludes any element whose whose string -->
<!-- *    value is identical to value of its url xlink:href attribute). -->
<!-- * -->
<!-- * 2. Puts a numbered marker inline to mark the place where the -->
<!-- *    notesource occurs in the main text flow. -->
<!-- * -->
<!-- * 3. Generates a numbered endnotes list (titled NOTES in English) -->
<!-- *    at the end of the man page, with the contents of each -->
<!-- *    notesource. -->
<!-- * -->
<!-- * Note that table footnotes are not listed in the endnotes list, -->
<!-- * and are not handled by this stylesheet (they are instead handled -->
<!-- * by the table.xsl stylesheet). -->
<!-- * -->
<!-- * Also, we don't get notesources in *info sections or Refmeta or -->
<!-- * Refnamediv or Indexterm, because, in manpages output, contents of -->
<!-- * those are either suppressed or are displayed out of document -->
<!-- * order - for example, the Info/Author content gets moved to the -->
<!-- * end of the page. So, if we were to number notesources in the -->
<!-- * Author content, it would "throw off" the numbering at the -->
<!-- * beginning of the main text flow. -->
<!-- * -->
<!-- * And for the record, one reason we don't use xsl:key to index the -->
<!-- * earmarks is that we need to get and check the sets of -->
<!-- * earmarks for uniqueness per-Refentry (not per-document). -->
<!-- * -->
<!-- * FIXME: as -->
<!-- * with "repeat" URLS, alt instances that have the same string value -->
<!-- * as preceding ones (likely to occur for repeat acroynyms and -->
<!-- * abbreviations) should be listed only once in the endnotes list, -->
<!-- * and numbered accordingly inline; split man.indent.width into -->
<!-- * man.indent.width.value (default 4) and man.indent.width.units -->
<!-- * (default n); also, if the first child of notesource is some block -->
<!-- * content other than a (non-formal) paragraph, the current code -->
<!-- * will probably end up generating a blank line after the -->
<!-- * corresponding number in the endnotes list... we should probably -->
<!-- * try to instead display the title of that block content there (if -->
<!-- * there is one: e.g., the list title, admonition title, etc.) -->

<!-- ==================================================================== -->

<xsl:template name="get.all.earmark.indexes.in.current.document">
  <!-- * Here we create a tree to hold indexes of all earmarks in -->
  <!-- * the current document. If the current document contains -->
  <!-- * multiple refentry instances, then this tree will contain -->
  <!-- * multiple indexes. -->
  <xsl:if test="$man.endnotes.are.numbered != 0">
    <!-- * Only create earmark indexes if user wants numbered endnotes -->
    <xsl:for-each select="//refentry">
      <earmark.index>
        <xsl:attribute name="idref">
          <xsl:value-of select="generate-id()"/>
        </xsl:attribute>
        <xsl:for-each
            select=".//*[self::*[@xlink:href]
                    or self::ulink
                    or self::imagedata
                    or self::audiodata
                    or self::videodata
                    or self::footnote[not(ancestor::table)]
                    or self::annotation
                    or self::alt]
                    [(node()
                      or self::imagedata
                      or self::audiodata
                      or self::videodata
                      )
                    and not(ancestor::refentryinfo)
                    and not(ancestor::info)
                    and not(ancestor::docinfo)
                    and not(ancestor::refmeta)
                    and not(ancestor::refnamediv)
                    and not(ancestor::indexterm)
                    and not(. = @url)
                    and not(. = @xlink:href)
                    and not(@url =
                    preceding::ulink[node()
                    and not(ancestor::refentryinfo)
                    and not(ancestor::info)
                    and not(ancestor::docinfo)
                    and not(ancestor::refmeta)
                    and not(ancestor::refnamediv)
                    and not(ancestor::indexterm)
                    and (generate-id(ancestor::refentry)
                    = generate-id(current()))]/@url)
                    and not(@xlink:href =
                    preceding::*[@xlink:href][node()
                    and not(ancestor::refentryinfo)
                    and not(ancestor::info)
                    and not(ancestor::docinfo)
                    and not(ancestor::refmeta)
                    and not(ancestor::refnamediv)
                    and not(ancestor::indexterm)
                    and (generate-id(ancestor::refentry)
                    = generate-id(current()))]/@xlink:href)
                    and not(@fileref =
                    preceding::*[@fileref][
                    not(ancestor::refentryinfo)
                    and not(ancestor::info)
                    and not(ancestor::docinfo)
                    and not(ancestor::refmeta)
                    and not(ancestor::refnamediv)
                    and not(ancestor::indexterm)
                    and (generate-id(ancestor::refentry)
                    = generate-id(current()))]/@fileref)]">
          <earmark>
            <xsl:attribute name="id">
              <xsl:value-of select="generate-id()"/>
            </xsl:attribute>
            <xsl:attribute name="number">
              <xsl:value-of select="position()"/>
            </xsl:attribute>
            <xsl:if test="@url|@xlink:href|@fileref">
              <!-- * Only add a uri attribute if the notesource is -->
              <!-- * a link or an element that references an external -->
              <!-- * (an imagedata, audiodata, or videodata element) -->
              <xsl:attribute name="uri">
                <xsl:value-of select="@url|@xlink:href|@fileref"/>
              </xsl:attribute>
            </xsl:if>
            <xsl:copy>
              <xsl:copy-of select="node()"/>
            </xsl:copy>
          </earmark>
        </xsl:for-each>
      </earmark.index>
    </xsl:for-each>
  </xsl:if>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="*[@xlink:href]|ulink
                     |imagedata|audiodata|videodata
                     |footnote[not(ancestor::table)]
                     |annotation|alt">
  <xsl:variable name="all.earmark.indexes.in.current.document.rtf">
    <xsl:call-template name="get.all.earmark.indexes.in.current.document"/>
  </xsl:variable>
  <xsl:variable name="all.earmark.indexes.in.current.document"
                select="exsl:node-set($all.earmark.indexes.in.current.document.rtf)"/>
  <xsl:variable name="all.earmarks.in.current.refentry.rtf">
    <!-- * get the set of all earmarks for the ancestor Refentry of -->
    <!-- * this notesource -->
    <xsl:copy-of
        select="$all.earmark.indexes.in.current.document/earmark.index
                [@idref =
                generate-id(current()/ancestor::refentry)]/earmark"/>
  </xsl:variable>
  <xsl:variable name="all.earmarks.in.current.refentry"
                select="exsl:node-set($all.earmarks.in.current.refentry.rtf)"/>

  <!-- * identify the earmark for the current element -->
  <xsl:variable name="earmark">
    <xsl:choose>
      <xsl:when test="@url|@xlink:href">
        <xsl:value-of select="@url|@xlink:href"/>
      </xsl:when>
      <xsl:when test="@fileref">
        <xsl:value-of select="@fileref"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="generate-id()"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="notesource.contents">
    <xsl:choose>
      <!-- * check to see if the element is empty or not; if it's -->
      <!-- * non-empty, get the content -->
      <xsl:when test="node()">
        <xsl:apply-templates/>
      </xsl:when>
      <xsl:otherwise>
        <!-- * Otherwise this is an empty link or an empty imagedata, -->
        <!-- * audiodata, or videodata element, so we just get the -->
        <!-- * value of its url, xlink:href, or fileref attribute. -->
        <!-- * -->
        <!-- * Add hyphenation suppression in URL output only if -->
        <!-- * break.after.slash is also non-zero -->
        <xsl:if test="$man.hyphenate.urls = 0 and
                      $man.break.after.slash = 0">
          <xsl:call-template name="suppress.hyphenation"/>
          <xsl:text>\%</xsl:text>
        </xsl:if>
        <xsl:value-of select="$earmark"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:if test="self::ulink or self::*[@xlink:href]">
    <!-- * This is a hyperlink, so we need to decide how to format -->
    <!-- * the inline contents of the link (to underline or not). -->
    <xsl:choose>
      <!-- * if user wants links underlined, underline (ital) it -->
      <xsl:when test="$man.links.are.underlined != 0">
        <xsl:variable name="link.wrapper">
          <italic><xsl:value-of select="$notesource.contents"/></italic>
        </xsl:variable>
        <xsl:apply-templates mode="italic"
                             select="exsl:node-set($link.wrapper)"/>
      </xsl:when>
      <xsl:otherwise>
        <!-- * user doesn't want links underlined, so just display content -->
        <xsl:value-of select="$notesource.contents"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:if>

  <!-- * Format the numbered marker for this notesource -->
  <!-- * -->
  <!-- * If this is an imagedata, audiodata, or videodata element -->
  <!-- * OR if it's a non-empty element AND its string value is not -->
  <!-- * equal to the value of its url or xlink:href attribute (if -->
  <!-- * it has one) AND user wants endnotes numbered, only then -->
  <!-- * do we output a number for it -->
  <xsl:if test="(self::imagedata or
                 self::audiodata or
                 self::videodata or
                  (node()
                    and not(. = @url)
                    and not(. = @xlink:href))
                )
                and $man.endnotes.are.numbered != 0">
    <!-- * To generate the numbered marker for this notesource, we -->
    <!-- * check the index of all earmarks for the current refentry -->
    <!-- * and find the number of the indexed earmark which matches -->
    <!-- * this notesource's earmark. -->
    <!-- * Note that multiple notesources may share the same -->
    <!-- * numbered earmark; in that case, they get the same number. -->
    <!-- * -->
    <xsl:variable name="notesource.number">
      <xsl:choose>
        <xsl:when test="self::ulink or
                        self::*[@xlink:href] or
                        self::imagedata or
                        self::audiodata or
                        self::videodata">
          <xsl:value-of select="$all.earmarks.in.current.refentry/earmark[@uri = $earmark]/@number"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$all.earmarks.in.current.refentry/earmark[@id  = $earmark]/@number"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <!-- * Format the number by placing it in square brackets. FIXME: -->
    <!-- * This formatting should probably be made user-configurable, -->
    <!-- * to allow something other than just square brackets; e.g., -->
    <!-- * Angle brackets<10> or Braces{10}  -->
    <xsl:text>\&amp;[</xsl:text>
    <xsl:value-of select="$notesource.number"/>
    <xsl:text>]</xsl:text>
    <!-- * Note that the reason for the \& before the opening bracket -->
    <!-- * is to prevent any possible linebreak from being introduced -->
    <!-- * between the opening bracket and the following text. -->
  </xsl:if>
  </xsl:template>

<!-- ==================================================================== -->

<xsl:template name="endnotes.list">
  <!-- We have stored earmark indexes for all refentry instances in the -->
  <!-- current document, with the ID for each index being the same ID as -->
  <!-- its corresponding refentry; so we now need to get the ID for the -->
  <!-- current refentry so we can grab its corresponding earmark index -->
  <xsl:variable name="current.refentry.id">
    <xsl:value-of select="generate-id(.)"/>
  </xsl:variable>

  <xsl:variable name="endnotes.rtf">
    <xsl:variable name="all.earmark.indexes.in.current.document.rtf">
      <xsl:call-template  name="get.all.earmark.indexes.in.current.document"/>
    </xsl:variable>
    <xsl:variable name="all.earmark.indexes.in.current.document"
                  select="exsl:node-set($all.earmark.indexes.in.current.document.rtf)"/>
      <xsl:copy-of
          select="$all.earmark.indexes.in.current.document/earmark.index
                  [@idref = $current.refentry.id]/earmark"/>
  </xsl:variable>

  <xsl:variable name="endnotes" select="exsl:node-set($endnotes.rtf)"/>

  <!-- * check to see if we have actually found any content to use as -->
  <!-- * endnotes; if we have, we generate the endnotes list, if not, -->
  <!-- * we do nothing -->
  <xsl:if test="$endnotes/node()">
    <xsl:call-template name="format.endnotes.list">
      <xsl:with-param name="endnotes" select="$endnotes"/>
    </xsl:call-template>
  </xsl:if>

</xsl:template>

<!-- ==================================================================== -->

<xsl:template name="format.endnotes.list">
  <xsl:param name="endnotes"/>
  <xsl:call-template name="mark.subheading"/>

  <!-- * ======= make the endnotes-list section heading ============= -->
  <xsl:text>.SH "</xsl:text>
  <xsl:call-template name="string.upper">
    <xsl:with-param name="string">
      <xsl:choose>
        <!-- * if user has specified a heading, use that -->
        <xsl:when test="$man.endnotes.list.heading != ''">
          <xsl:value-of select="$man.endnotes.list.heading"/>
        </xsl:when>
        <xsl:otherwise>
          <!-- * otherwise, get localized heading from gentext -->
          <!-- * (in English, NOTES) -->
          <xsl:call-template name="gentext">
            <xsl:with-param name="key" select="'Notes'"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:with-param>
  </xsl:call-template>
  <xsl:text>"&#10;</xsl:text>

  <!-- * ================ process each earmark ====================== -->
  <xsl:for-each select="$endnotes/earmark">
    <!-- * make paragraph with hanging indent, and starting with a -->
    <!-- * number in the form " 1." (padded to $man.indent.width - 1) -->
    <xsl:text>.IP</xsl:text>
    <xsl:text> "</xsl:text>
    <xsl:variable name="endnote.number">
      <xsl:value-of select="@number"/>
      <xsl:text>.</xsl:text>
    </xsl:variable>
    <xsl:call-template name="prepend-pad">
      <xsl:with-param name="padVar" select="$endnote.number"/>
      <!-- FIXME: the following assumes that $man.indent.width is in -->
      <!-- en's; also, this should probably use $list.indent instead -->
      <xsl:with-param name="length" select="$man.indent.width - 1"/>
    </xsl:call-template>
    <xsl:text>"</xsl:text>
    <xsl:if test="not($list-indent = '')">
      <xsl:text> </xsl:text>
      <xsl:value-of select="$list-indent"/>
    </xsl:if>
    <xsl:text>&#10;</xsl:text>

    <!-- * ========================================================= -->
    <!-- *           print the notesource/endnote contents -->
    <!-- * ========================================================= -->
    <xsl:choose>
      <xsl:when test="*/node()">
        <!-- * if the earmark has non-empty child content, then -->
        <!-- * its corresponding notesource is either a link or -->
        <!-- * an instance of annotative text, so we want to -->
        <!-- * display that content -->
        <xsl:apply-templates select="*/node()"/>
      </xsl:when>
      <xsl:otherwise>
        <!-- * otherwise, this earmark has empty content, -->
        <!-- * which means its corresponding notesources is an -->
        <!-- * imagedata, audiodata, or videodata instance; in -->
        <!-- * that case, we use the value of the notesoures's -->
        <!-- * @fileref attribute (which is stored in the -->
        <!-- * earmark uri attribute) as the "contents" for -->
        <!-- * this endnote/notesource -->
        <xsl:value-of select="@uri"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>&#10;</xsl:text>

    <!-- * ========================================================= -->
    <!-- *           print the URL for links -->
    <!-- * ========================================================= -->
    <!-- * In addition to the notesource contents, if the -->
    <!-- * notesource is a link, we display the URL for the link. -->
    <!-- * But for notesources that are imagedata, audiodata, or -->
    <!-- * videodata instances, we don't want to (re)display the -->
    <!-- * URL for those here, because for those elements, the -->
    <!-- * notesource contents are the URL (the value of the -->
    <!-- * @fileref attribute), and we have already rendered them. -->
    <!-- * -->
    <!-- * We know an earmark is a link if it has non-empty child -->
    <!-- * content and a uri attribute; so we check for that -->
    <!-- * condition here. -->
    <xsl:if test="*/node() and @uri">
      <xsl:text>.RS</xsl:text>
      <xsl:if test="not($list-indent = '')">
        <xsl:text> </xsl:text>
        <xsl:value-of select="$list-indent"/>
      </xsl:if>
      <xsl:text>&#10;</xsl:text>
      <!-- * Add hyphenation suppression in URL output only if -->
      <!-- * $break.after.slash is also non-zero -->
      <xsl:if test="$man.hyphenate.urls = 0
                    and $man.break.after.slash = 0">
        <xsl:call-template name="suppress.hyphenation"/>
        <xsl:text>\%</xsl:text>
      </xsl:if>
      <xsl:value-of select="@uri"/>
      <xsl:text>&#10;</xsl:text>
      <xsl:text>.RE</xsl:text>
      <xsl:text>&#10;</xsl:text>
    </xsl:if>

  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
