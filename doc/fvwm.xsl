<?xml version='1.0'?>
<!--
	$Id$

	Author: Scott Smedley
-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version='1.0'>

<xsl:import href="docbook-xsl/html/docbook.xsl"/>

<!-- twdt = The Whole Damn Thing ... ie. everything on a single page -->
<xsl:param name="twdt" select="''"/>
<xsl:param name="header.file" select="'../header.html'"/>
<xsl:param name="footer.file" select="'../footer.html'"/>

<!-- fvwmref -->
<xsl:template match="fvwmref">
	<a>
	<xsl:attribute name="href">
		<xsl:choose>
			<xsl:when test="($twdt = 1) and not (@mod)">
				<xsl:text>#</xsl:text>
				<xsl:choose>
					<xsl:when test="@opt">
						<xsl:choose>
							<xsl:when test="@cmd">
								<xsl:value-of select="@cmd"/>
								<xsl:text>_</xsl:text>
								<xsl:value-of select="@opt"/>
							</xsl:when>
							<!-- mod=".." opt=".." isn't used. -->
							<xsl:otherwise>
								<xsl:value-of select="@opt"/>
							</xsl:otherwise>
						</xsl:choose>
					</xsl:when>
					<xsl:when test="@cmd">
						<xsl:value-of select="@cmd"/>
					</xsl:when>
					<xsl:when test="@mod">
						<xsl:value-of select="@mod"/>
					</xsl:when>
				</xsl:choose>
			</xsl:when>
			<xsl:otherwise>
				<xsl:choose>
					<xsl:when test="@cmd">
						<xsl:text>../commands/</xsl:text>
						<xsl:value-of select="@cmd"/>
					</xsl:when>
					<xsl:when test="@mod">
						<xsl:text>../modules/</xsl:text>
						<xsl:value-of select="@mod"/>
					</xsl:when>
					<xsl:when test="@sect">
						<xsl:text>../fvwm/</xsl:text>
						<xsl:value-of select="@sect"/>
					</xsl:when>
					<!-- anything else should be an error -->
				</xsl:choose>
				<xsl:text>.html</xsl:text>
				<xsl:if test="@opt">
					<xsl:text>#</xsl:text>
					<xsl:choose>
						<xsl:when test="@cmd">
							<xsl:value-of select="@cmd"/>
							<xsl:text>_</xsl:text>
						</xsl:when>
						<xsl:when test="@mod">
							<xsl:value-of select="@mod"/>
							<xsl:text>_</xsl:text>
						</xsl:when>
					</xsl:choose>
					<xsl:value-of select="@opt"/>
				</xsl:if>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:attribute>
	<xsl:choose>
		<xsl:when test="@sect">
			<xsl:value-of select="@name"/>
		</xsl:when>
		<xsl:when test="@opt">
			<xsl:value-of select="@opt"/>
		</xsl:when>
		<xsl:otherwise>
			<xsl:if test="@cmd">
				<xsl:value-of select="@cmd"/>
			</xsl:if>
			<xsl:if test="@mod">
				<xsl:value-of select="@mod"/>
			</xsl:if>
		</xsl:otherwise>
	</xsl:choose>
	</a>
</xsl:template>

<!-- fvwmopt -->
<xsl:template match="fvwmopt">
	<a>
	<xsl:attribute name="name">
		<xsl:choose>
			<xsl:when test="@cmd">
				<xsl:value-of select="@cmd"/>
			</xsl:when>
			<xsl:when test="@mod">
				<xsl:value-of select="@mod"/>
			</xsl:when>
		</xsl:choose>
		<xsl:text>_</xsl:text>
		<xsl:value-of select="@opt"/>
	</xsl:attribute>
	</a>
	<font class="fvwmopt"><xsl:value-of select="@opt"/></font>
</xsl:template>

<xsl:template name="user.header.content">
	<xsl:variable name="codefile" select="document($header.file,/)"/>
	<xsl:copy-of select="$codefile/*/node()"/>
</xsl:template>

<xsl:template name="user.footer.content">
	<xsl:variable name="codefile" select="document($footer.file,/)"/>
	<xsl:copy-of select="$codefile/*/node()"/>
</xsl:template>


</xsl:stylesheet>
