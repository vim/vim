" Vim syntax file
" Language:          ReDIF
" Maintainer:        Axel Castellane <axel.castellane@polytechnique.edu>
" Last Change:       2013 Feb 20
" Original Author:   Axel Castellane
" Source:            http://openlib.org/acmes/root/docu/redif_1.html
" Note:              The ReDIF format is used by RePEc.

" Start with a check for "b:current_syntax".  If it is defined, some other
" syntax file, earlier in 'runtimepath' was already loaded:
if exists("b:current_syntax")
  finish
endif

" Sync: The template-type (ReDIF-Paper, ReDIF-Archive, etc.) influences which
" fields can follow. Thus sync must start from the beginning to know which
" fields are right or wrong.
syntax sync fromstart

" ReDIF is case-insensitive
syntax case ignore

" Structure: Some fields determine what fields can come next. For example:
"       Template-Type
"       *-Name
"       File-URL
"       *-Institution
" Those fields span a syntax region over several lines so that these regions
" can only contain their respective items.
"
" Other fields (except comments) can only happen in one of these regions.

" Comments must start with # as the first character of the line, otherwise
" I believe that they are considered as part of an argument.
syntax region redifComment start=/^#/ end=/$/ containedin=ALL display

" Beginning: Anything which is not a comment is not allowed before a
" "Template-Type:" statement.  However, not to trouble the users when beginning
" writing a Template-Type, this are not highlighted as errors.
syntax region redifIncorrectBeginningOfFile start=/\%^/ end=/^Template-Type:\_s*ReDIF-\%(Paper\|Article\|Chapter\|Book\|Software\|Archive\|Series\|Institution\|Person\) \d\+\.\d\+/me=s-1 contains=redifComment,redifBeginningOfCorrectTemplateType,redifFieldTemplateType
syntax match redifBeginningOfCorrectTemplateType /^\cT\%[emplate-Type:]$/ display

highlight def link redifIncorrectBeginningOfFile redifError

" Defines the 9 possible multi-lines regions of Template-Type and the fields
" they can contain.
syntax region redifRegionTemplatePaper start=/^Template-Type:\_s*ReDIF-Paper \d\+\.\d\+/ end=/^Template-Type:\_s*ReDIF-\%(Paper\|Article\|Chapter\|Book\|Software\|Archive\|Series\|Institution\|Person\) \d\+\.\d\+/me=s-1 contains=redifUnknownField,redifRegionClusterAuthor,redifRegionClusterFile,redifFieldTitle,redifFieldHandleOfWork,redifFieldLanguage,redifFieldContactEmail,redifFieldAbstract,redifFieldClassificationJEL,redifFieldKeywords,redifFieldNumber,redifFieldCreationDate,redifFieldRevisionDate,redifFieldPublicationStatus,redifFieldNote,redifFieldLength,redifFieldSeries,redifFieldAvailability,redifFieldOrderURL,redifFieldArticleHandle,redifFieldBookHandle,redifFieldChapterHandle,redifFieldPaperHandle,redifFieldSoftwareHandle,redifFieldRestriction,redifFieldPrice,redifFieldNotification,redifFieldPublicationType,redifFieldTemplateType
syntax region redifRegionTemplateArticle start=/^Template-Type:\_s*ReDIF-Article \d\+\.\d\+/ end=/^Template-Type:\_s*ReDIF-\%(Paper\|Article\|Chapter\|Book\|Software\|Archive\|Series\|Institution\|Person\) \d\+\.\d\+/me=s-1 contains=redifUnknownField,redifRegionClusterAuthor,redifRegionClusterFile,redifFieldTitle,redifFieldHandleOfWork,redifFieldLanguage,redifFieldContactEmail,redifFieldAbstract,redifFieldClassificationJEL,redifFieldKeywords,redifFieldNumber,redifFieldCreationDate,redifFieldPublicationStatus,redifFieldOrderURL,redifFieldArticleHandle,redifFieldBookHandle,redifFieldChapterHandle,redifFieldPaperHandle,redifFieldSoftwareHandle,redifFieldRestriction,redifFieldPrice,redifFieldNotification,redifFieldPublicationType,redifFieldJournal,redifFieldVolume,redifFieldYear,redifFieldIssue,redifFieldMonth,redifFieldPages,redifFieldNumber,redifFieldArticleHandle,redifFieldBookHandle,redifFieldChapterHandle,redifFieldPaperHandle,redifFieldSoftwareHandle,redifFieldTemplateType
syntax region redifRegionTemplateChapter start=/^Template-Type:\_s*ReDIF-Chapter \d\+\.\d\+/ end=/^Template-Type:\_s*ReDIF-\%(Paper\|Article\|Chapter\|Book\|Software\|Archive\|Series\|Institution\|Person\) \d\+\.\d\+/me=s-1 contains=redifUnknownField,redifRegionClusterAuthor,redifRegionClusterFile,redifRegionClusterProvider,redifRegionClusterPublisher,redifRegionClusterEditor,redifFieldHandleOfWork,redifFieldTitle,redifFieldContactEmail,redifFieldAbstract,redifFieldClassificationJEL,redifFieldKeywords,redifFieldBookTitle,redifFieldYear,redifFieldMonth,redifFieldPages,redifFieldChapter,redifFieldVolume,redifFieldEdition,redifFieldSeries,redifFieldISBN,redifFieldPublicationStatus,redifFieldNote,redifFieldInBook,redifFieldOrderURL,redifFieldArticleHandle,redifFieldBookHandle,redifFieldChapterHandle,redifFieldPaperHandle,redifFieldSoftwareHandle,redifFieldTemplateType
syntax region redifRegionTemplateBook start=/^Template-Type:\_s*ReDIF-Book \d\+\.\d\+/ end=/^Template-Type:\_s*ReDIF-\%(Paper\|Article\|Chapter\|Book\|Software\|Archive\|Series\|Institution\|Person\) \d\+\.\d\+/me=s-1 contains=redifUnknownField,redifRegionClusterAuthor,redifRegionClusterFile,redifRegionClusterProvider,redifRegionClusterPublisher,redifRegionClusterEditor,redifFieldTitle,redifFieldHandleOfWork,redifFieldContactEmail,redifFieldYear,redifFieldMonth,redifFieldVolume,redifFieldEdition,redifFieldSeries,redifFieldISBN,redifFieldPublicationStatus,redifFieldNote,redifFieldAbstract,redifFieldClassificationJEL,redifFieldKeywords,redifFieldHasChapter,redifFieldPrice,redifFieldOrderURL,redifFieldNumber,redifFieldCreationDate,redifFieldPublicationDate,redifFieldArticleHandle,redifFieldBookHandle,redifFieldChapterHandle,redifFieldPaperHandle,redifFieldSoftwareHandle,redifFieldTemplateType
syntax region redifRegionTemplateSoftware start=/^Template-Type:\_s*ReDIF-Software \d\+\.\d\+/ end=/^Template-Type:\_s*ReDIF-\%(Paper\|Article\|Chapter\|Book\|Software\|Archive\|Series\|Institution\|Person\) \d\+\.\d\+/me=s-1 contains=redifUnknownField,redifRegionClusterAuthor,redifRegionClusterFile,redifFieldHandleOfWork,redifFieldTitle,redifFieldProgrammingLanguage,redifFieldAbstract,redifFieldNumber,redifFieldVersion,redifFieldClassificationJEL,redifFieldKeywords,redifFieldSize,redifFieldSeries,redifFieldCreationDate,redifFieldRevisionDate,redifFieldNote,redifFieldRequires,redifFieldArticleHandle,redifFieldBookHandle,redifFieldChapterHandle,redifFieldPaperHandle,redifFieldSoftwareHandle,redifFieldTemplateType
syntax region redifRegionTemplateArchive start=/^Template-Type:\_s*ReDIF-Archive \d\+\.\d\+/ end=/^Template-Type:\_s*ReDIF-\%(Paper\|Article\|Chapter\|Book\|Software\|Archive\|Series\|Institution\|Person\) \d\+\.\d\+/me=s-1 contains=redifUnknownField,redifFieldHandleOfArchive,redifFieldURL,redifFieldMaintainerEmail,redifFieldName,redifFieldMaintainerName,redifFieldMaintainerPhone,redifFieldMaintainerFax,redifFieldClassificationJEL,redifFieldHomepage,redifFieldDescription,redifFieldNotification,redifFieldRestriction,redifFieldTemplateType
syntax region redifRegionTemplateSeries start=/^Template-Type:\_s*ReDIF-Series \d\+\.\d\+/ end=/^Template-Type:\_s*ReDIF-\%(Paper\|Article\|Chapter\|Book\|Software\|Archive\|Series\|Institution\|Person\) \d\+\.\d\+/me=s-1 contains=redifUnknownField,redifRegionClusterProvider,redifRegionClusterPublisher,redifRegionClusterEditor,redifFieldName,redifFieldHandleOfSeries,redifFieldMaintainerEmail,redifFieldType,redifFieldOrderEmail,redifFieldOrderHomepage,redifFieldOrderPostal,redifFieldPrice,redifFieldRestriction,redifFieldMaintainerPhone,redifFieldMaintainerFax,redifFieldMaintainerName,redifFieldDescription,redifFieldClassificationJEL,redifFieldKeywords,redifFieldNotification,redifFieldISSN,redifFieldFollowup,redifFieldPredecessor,redifFieldTemplateType
syntax region redifRegionTemplateInstitution start=/^Template-Type:\_s*ReDIF-Institution \d\+\.\d\+/ end=/^Template-Type:\_s*ReDIF-\%(Paper\|Article\|Chapter\|Book\|Software\|Archive\|Series\|Institution\|Person\) \d\+\.\d\+/me=s-1 contains=redifUnknownField,redifRegionClusterPrimary,redifRegionClusterSecondary,redifRegionClusterTertiary,redifRegionClusterQuaternary,redifFieldHandleOfInstitution,redifFieldPrimaryDefunct,redifFieldSecondaryDefunct,redifFieldTertiaryDefunct,redifFieldTemplateType
syntax region redifRegionTemplatePerson start=/^Template-Type:\_s*ReDIF-Person \d\+\.\d\+/ end=/^Template-Type:\_s*ReDIF-\%(Paper\|Article\|Chapter\|Book\|Software\|Archive\|Series\|Institution\|Person\) \d\+\.\d\+/me=s-1 contains=redifUnknownField,redifRegionClusterWorkplace,redifFieldHandleOfPerson,redifFieldNameFull,redifFieldNameFirst,redifFieldNameLast,redifFieldNamePrefix,redifFieldNameMiddle,redifFieldNameSuffix,redifFieldNameASCII,redifFieldEmail,redifFieldHomepage,redifFieldFax,redifFieldPostal,redifFieldPhone,redifFieldWorkplaceOrganization,redifFieldAuthorPaper,redifFieldAuthorArticle,redifFieldAuthorSoftware,redifFieldAuthorBook,redifFieldAuthorChapter,redifFieldEditorBook,redifFieldEditorSeries,redifFieldClassificationJEL,redifFieldShortId,redifFieldLastLoginDate,redifFieldRegisteredDate,redifFieldTemplateType

" For each template, non-whitespaces ending with a colon must be
" correct fields. By default, they are wrong fields.
syntax match redifUnknownField /^\S\{-1,}:/ contained display

highlight def link redifUnknownField redifError

" Defines the 12 possible clusters and what they can contain
syntax region redifRegionClusterAuthorWorkplace start=/^Author-Workplace-Name:/ skip=/^Author-Workplace-\%(Name-English\|Homepage\|Postal\|Location\|Email\|Phone\|Fax\|Institution\):/ end=/^\c\S\{-1,}:/me=s-1 contained contains=redifFieldAuthorWorkplaceName,redifFieldAuthorWorkplaceHomepage,redifFieldAuthorWorkplaceNameEnglish,redifFieldAuthorWorkplacePostal,redifFieldAuthorWorkplaceLocation,redifFieldAuthorWorkplaceEmail,redifFieldAuthorWorkplacePhone,redifFieldAuthorWorkplaceFax,redifFieldAuthorWorkplaceInstitution
syntax region redifRegionClusterEditorWorkplace start=/^Editor-Workplace-Name:/ skip=/^Editor-Workplace-\%(Name-English\|Homepage\|Postal\|Location\|Email\|Phone\|Fax\|Institution\):/ end=/^\c\S\{-1,}:/me=s-1 contained contains=redifFieldEditorWorkplaceName,redifFieldEditorWorkplaceHomepage,redifFieldEditorWorkplaceNameEnglish,redifFieldEditorWorkplacePostal,redifFieldEditorWorkplaceLocation,redifFieldEditorWorkplaceEmail,redifFieldEditorWorkplacePhone,redifFieldEditorWorkplaceFax,redifFieldEditorWorkplaceInstitution
syntax region redifRegionClusterWorkplace start=/^Workplace-Name:/ skip=/^Workplace-\%(Name-English\|Homepage\|Postal\|Location\|Email\|Phone\|Fax\|Institution\):/ end=/^\c\S\{-1,}:/me=s-1 contained contains=redifFieldWorkplaceName,redifFieldWorkplaceHomepage,redifFieldWorkplaceNameEnglish,redifFieldWorkplacePostal,redifFieldWorkplaceLocation,redifFieldWorkplaceEmail,redifFieldWorkplacePhone,redifFieldWorkplaceFax,redifFieldWorkplaceInstitution
syntax region redifRegionClusterPrimary start=/^Primary-Name:/ skip=/^Primary-\%(Name-English\|Homepage\|Postal\|Location\|Email\|Phone\|Fax\|Institution\):/ end=/^\c\S\{-1,}:/me=s-1 contained contains=redifFieldPrimaryName,redifFieldPrimaryHomepage,redifFieldPrimaryNameEnglish,redifFieldPrimaryPostal,redifFieldPrimaryLocation,redifFieldPrimaryEmail,redifFieldPrimaryPhone,redifFieldPrimaryFax,redifFieldPrimaryInstitution
syntax region redifRegionClusterSecondary start=/^Secondary-Name:/ skip=/^Secondary-\%(Name-English\|Homepage\|Postal\|Location\|Email\|Phone\|Fax\|Institution\):/ end=/^\c\S\{-1,}:/me=s-1 contained contains=redifFieldSecondaryName,redifFieldSecondaryHomepage,redifFieldSecondaryNameEnglish,redifFieldSecondaryPostal,redifFieldSecondaryLocation,redifFieldSecondaryEmail,redifFieldSecondaryPhone,redifFieldSecondaryFax,redifFieldSecondaryInstitution
syntax region redifRegionClusterTertiary start=/^Tertiary-Name:/ skip=/^Tertiary-\%(Name-English\|Homepage\|Postal\|Location\|Email\|Phone\|Fax\|Institution\):/ end=/^\c\S\{-1,}:/me=s-1 contained contains=redifFieldTertiaryName,redifFieldTertiaryHomepage,redifFieldTertiaryNameEnglish,redifFieldTertiaryPostal,redifFieldTertiaryLocation,redifFieldTertiaryEmail,redifFieldTertiaryPhone,redifFieldTertiaryFax,redifFieldTertiaryInstitution
syntax region redifRegionClusterQuaternary start=/^Quaternary-Name:/ skip=/^Quaternary-\%(Name-English\|Homepage\|Postal\|Location\|Email\|Phone\|Fax\|Institution\):/ end=/^\c\S\{-1,}:/me=s-1 contained contains=redifFieldQuaternaryName,redifFieldQuaternaryHomepage,redifFieldQuaternaryNameEnglish,redifFieldQuaternaryPostal,redifFieldQuaternaryLocation,redifFieldQuaternaryEmail,redifFieldQuaternaryPhone,redifFieldQuaternaryFax,redifFieldQuaternaryInstitution
syntax region redifRegionClusterProvider start=/^Provider-Name:/ skip=/^Provider-\%(Name-English\|Homepage\|Postal\|Location\|Email\|Phone\|Fax\|Institution\):/ end=/^\c\S\{-1,}:/me=s-1 contained contains=redifFieldProviderName,redifFieldProviderHomepage,redifFieldProviderNameEnglish,redifFieldProviderPostal,redifFieldProviderLocation,redifFieldProviderEmail,redifFieldProviderPhone,redifFieldProviderFax,redifFieldProviderInstitution
syntax region redifRegionClusterPublisher start=/^Publisher-Name:/ skip=/^Publisher-\%(Name-English\|Homepage\|Postal\|Location\|Email\|Phone\|Fax\|Institution\):/ end=/^\c\S\{-1,}:/me=s-1 contained contains=redifFieldPublisherName,redifFieldPublisherHomepage,redifFieldPublisherNameEnglish,redifFieldPublisherPostal,redifFieldPublisherLocation,redifFieldPublisherEmail,redifFieldPublisherPhone,redifFieldPublisherFax,redifFieldPublisherInstitution
syntax region redifRegionClusterAuthor start=/^Author-Name:/ skip=/^Author-\%(Name\%(-First\|-Last\)\|Homepage\|Email\|Fax\|Postal\|Phone\|Person\|Workplace-Name\):/ end=/^\c\S\{-1,}:/me=s-1 contained contains=redifRegionClusterAuthorWorkplace,redifFieldAuthorName,redifFieldAuthorNameFirst,redifFieldAuthorNameLast,redifFieldAuthorHomepage,redifFieldAuthorEmail,redifFieldAuthorFax,redifFieldAuthorPostal,redifFieldAuthorPhone,redifFieldAuthorPerson
syntax region redifRegionClusterEditor start=/^Editor-Name:/ skip=/^Editor-\%(Name\%(-First\|-Last\)\|Homepage\|Email\|Fax\|Postal\|Phone\|Person\|Workplace-Name\):/ end=/^\c\S\{-1,}:/me=s-1 contained contains=redifRegionClusterEditorWorkplace,redifFieldEditorName,redifFieldEditorNameFirst,redifFieldEditorNameLast,redifFieldEditorHomepage,redifFieldEditorEmail,redifFieldEditorFax,redifFieldEditorPostal,redifFieldEditorPhone,redifFieldEditorPerson
syntax region redifRegionClusterFile start=/^File-URL:/ skip=/^File-\%(Format\|Function\|Size\|Restriction\):/ end=/^\c\S\{-1,}:/me=s-1 contained contains=redifFieldFileURL,redifFieldFileFormat,redifFieldFileFunction,redifFieldFileSize,redifFieldFileRestriction

" A field not in the cluster ends the cluster, so no need to define wrong
" fields, because they will be handled by the containing UnknownField region
" immediately when exiting the cluster.

" All the possible fields
" Note: The "Handle" field is handled a little bit differently, because it
" does not have the same meaning depending on the Template-Type.
" 	/redifFieldHandleOf....
syntax match redifFieldAbstract /^Abstract:/ skipwhite skipempty nextgroup=redifArgumentAbstract contained
syntax match redifFieldArticleHandle /^Article-Handle:/ skipwhite skipempty nextgroup=redifArgumentArticleHandle contained
syntax match redifFieldAuthorArticle /^Author-Article:/ skipwhite skipempty nextgroup=redifArgumentAuthorArticle contained
syntax match redifFieldAuthorBook /^Author-Book:/ skipwhite skipempty nextgroup=redifArgumentAuthorBook contained
syntax match redifFieldAuthorChapter /^Author-Chapter:/ skipwhite skipempty nextgroup=redifArgumentAuthorChapter contained
syntax match redifFieldAuthorEmail /^Author-Email:/ skipwhite skipempty nextgroup=redifArgumentAuthorEmail contained
syntax match redifFieldAuthorFax /^Author-Fax:/ skipwhite skipempty nextgroup=redifArgumentAuthorFax contained
syntax match redifFieldAuthorHomepage /^Author-Homepage:/ skipwhite skipempty nextgroup=redifArgumentAuthorHomepage contained
syntax match redifFieldAuthorName /^Author-Name:/ skipwhite skipempty nextgroup=redifArgumentAuthorName contained
syntax match redifFieldAuthorNameFirst /^Author-Name-First:/ skipwhite skipempty nextgroup=redifArgumentAuthorNameFirst contained
syntax match redifFieldAuthorNameLast /^Author-Name-Last:/ skipwhite skipempty nextgroup=redifArgumentAuthorNameLast contained
syntax match redifFieldAuthorPaper /^Author-Paper:/ skipwhite skipempty nextgroup=redifArgumentAuthorPaper contained
syntax match redifFieldAuthorPerson /^Author-Person:/ skipwhite skipempty nextgroup=redifArgumentAuthorPerson contained
syntax match redifFieldAuthorPhone /^Author-Phone:/ skipwhite skipempty nextgroup=redifArgumentAuthorPhone contained
syntax match redifFieldAuthorPostal /^Author-Postal:/ skipwhite skipempty nextgroup=redifArgumentAuthorPostal contained
syntax match redifFieldAuthorSoftware /^Author-Software:/ skipwhite skipempty nextgroup=redifArgumentAuthorSoftware contained
syntax match redifFieldAuthorWorkplaceEmail /^Author-Workplace-Email:/ skipwhite skipempty nextgroup=redifArgumentAuthorWorkplaceEmail contained
syntax match redifFieldAuthorWorkplaceFax /^Author-Workplace-Fax:/ skipwhite skipempty nextgroup=redifArgumentAuthorWorkplaceFax contained
syntax match redifFieldAuthorWorkplaceHomepage /^Author-Workplace-Homepage:/ skipwhite skipempty nextgroup=redifArgumentAuthorWorkplaceHomepage contained
syntax match redifFieldAuthorWorkplaceInstitution /^Author-Workplace-Institution:/ skipwhite skipempty nextgroup=redifArgumentAuthorWorkplaceInstitution contained
syntax match redifFieldAuthorWorkplaceLocation /^Author-Workplace-Location:/ skipwhite skipempty nextgroup=redifArgumentAuthorWorkplaceLocation contained
syntax match redifFieldAuthorWorkplaceName /^Author-Workplace-Name:/ skipwhite skipempty nextgroup=redifArgumentAuthorWorkplaceName contained
syntax match redifFieldAuthorWorkplaceNameEnglish /^Author-Workplace-Name-English:/ skipwhite skipempty nextgroup=redifArgumentAuthorWorkplaceNameEnglish contained
syntax match redifFieldAuthorWorkplacePhone /^Author-Workplace-Phone:/ skipwhite skipempty nextgroup=redifArgumentAuthorWorkplacePhone contained
syntax match redifFieldAuthorWorkplacePostal /^Author-Workplace-Postal:/ skipwhite skipempty nextgroup=redifArgumentAuthorWorkplacePostal contained
syntax match redifFieldAvailability /^Availability:/ skipwhite skipempty nextgroup=redifArgumentAvailability contained
syntax match redifFieldBookHandle /^Book-Handle:/ skipwhite skipempty nextgroup=redifArgumentBookHandle contained
syntax match redifFieldBookTitle /^Book-Title:/ skipwhite skipempty nextgroup=redifArgumentBookTitle contained
syntax match redifFieldChapterHandle /^Chapter-Handle:/ skipwhite skipempty nextgroup=redifArgumentChapterHandle contained
syntax match redifFieldChapter /^Chapter:/ skipwhite skipempty nextgroup=redifArgumentChapter contained
syntax match redifFieldClassificationJEL /^Classification-JEL:/ skipwhite skipempty nextgroup=redifArgumentClassificationJEL contained
syntax match redifFieldContactEmail /^Contact-Email:/ skipwhite skipempty nextgroup=redifArgumentContactEmail contained
syntax match redifFieldCreationDate /^Creation-Date:/ skipwhite skipempty nextgroup=redifArgumentCreationDate contained
syntax match redifFieldDescription /^Description:/ skipwhite skipempty nextgroup=redifArgumentDescription contained
syntax match redifFieldEdition /^Edition:/ skipwhite skipempty nextgroup=redifArgumentEdition contained
syntax match redifFieldEditorBook /^Editor-Book:/ skipwhite skipempty nextgroup=redifArgumentEditorBook contained
syntax match redifFieldEditorEmail /^Editor-Email:/ skipwhite skipempty nextgroup=redifArgumentEditorEmail contained
syntax match redifFieldEditorFax /^Editor-Fax:/ skipwhite skipempty nextgroup=redifArgumentEditorFax contained
syntax match redifFieldEditorHomepage /^Editor-Homepage:/ skipwhite skipempty nextgroup=redifArgumentEditorHomepage contained
syntax match redifFieldEditorName /^Editor-Name:/ skipwhite skipempty nextgroup=redifArgumentEditorName contained
syntax match redifFieldEditorNameFirst /^Editor-Name-First:/ skipwhite skipempty nextgroup=redifArgumentEditorNameFirst contained
syntax match redifFieldEditorNameLast /^Editor-Name-Last:/ skipwhite skipempty nextgroup=redifArgumentEditorNameLast contained
syntax match redifFieldEditorPerson /^Editor-Person:/ skipwhite skipempty nextgroup=redifArgumentEditorPerson contained
syntax match redifFieldEditorPhone /^Editor-Phone:/ skipwhite skipempty nextgroup=redifArgumentEditorPhone contained
syntax match redifFieldEditorPostal /^Editor-Postal:/ skipwhite skipempty nextgroup=redifArgumentEditorPostal contained
syntax match redifFieldEditorSeries /^Editor-Series:/ skipwhite skipempty nextgroup=redifArgumentEditorSeries contained
syntax match redifFieldEditorWorkplaceEmail /^Editor-Workplace-Email:/ skipwhite skipempty nextgroup=redifArgumentEditorWorkplaceEmail contained
syntax match redifFieldEditorWorkplaceFax /^Editor-Workplace-Fax:/ skipwhite skipempty nextgroup=redifArgumentEditorWorkplaceFax contained
syntax match redifFieldEditorWorkplaceHomepage /^Editor-Workplace-Homepage:/ skipwhite skipempty nextgroup=redifArgumentEditorWorkplaceHomepage contained
syntax match redifFieldEditorWorkplaceInstitution /^Editor-Workplace-Institution:/ skipwhite skipempty nextgroup=redifArgumentEditorWorkplaceInstitution contained
syntax match redifFieldEditorWorkplaceLocation /^Editor-Workplace-Location:/ skipwhite skipempty nextgroup=redifArgumentEditorWorkplaceLocation contained
syntax match redifFieldEditorWorkplaceName /^Editor-Workplace-Name:/ skipwhite skipempty nextgroup=redifArgumentEditorWorkplaceName contained
syntax match redifFieldEditorWorkplaceNameEnglish /^Editor-Workplace-Name-English:/ skipwhite skipempty nextgroup=redifArgumentEditorWorkplaceNameEnglish contained
syntax match redifFieldEditorWorkplacePhone /^Editor-Workplace-Phone:/ skipwhite skipempty nextgroup=redifArgumentEditorWorkplacePhone contained
syntax match redifFieldEditorWorkplacePostal /^Editor-Workplace-Postal:/ skipwhite skipempty nextgroup=redifArgumentEditorWorkplacePostal contained
syntax match redifFieldEmail /^Email:/ skipwhite skipempty nextgroup=redifArgumentEmail contained
syntax match redifFieldFax /^Fax:/ skipwhite skipempty nextgroup=redifArgumentFax contained
syntax match redifFieldFileFormat /^File-Format:/ skipwhite skipempty nextgroup=redifArgumentFileFormat contained
syntax match redifFieldFileFunction /^File-Function:/ skipwhite skipempty nextgroup=redifArgumentFileFunction contained
syntax match redifFieldFileRestriction /^File-Restriction:/ skipwhite skipempty nextgroup=redifArgumentFileRestriction contained
syntax match redifFieldFileSize /^File-Size:/ skipwhite skipempty nextgroup=redifArgumentFileSize contained
syntax match redifFieldFileURL /^File-URL:/ skipwhite skipempty nextgroup=redifArgumentFileURL contained
syntax match redifFieldFollowup /^Followup:/ skipwhite skipempty nextgroup=redifArgumentFollowup contained
syntax match redifFieldHandleOfArchive /^Handle:/ skipwhite skipempty nextgroup=redifArgumentHandleOfArchive contained
syntax match redifFieldHandleOfInstitution /^Handle:/ skipwhite skipempty nextgroup=redifArgumentHandleOfInstitution contained
syntax match redifFieldHandleOfPerson /^Handle:/ skipwhite skipempty nextgroup=redifArgumentHandleOfPerson contained
syntax match redifFieldHandleOfSeries /^Handle:/ skipwhite skipempty nextgroup=redifArgumentHandleOfSeries contained
syntax match redifFieldHandleOfWork /^Handle:/ skipwhite skipempty nextgroup=redifArgumentHandleOfWork contained
syntax match redifFieldHasChapter /^HasChapter:/ skipwhite skipempty nextgroup=redifArgumentHasChapter contained
syntax match redifFieldHomepage /^Homepage:/ skipwhite skipempty nextgroup=redifArgumentHomepage contained
syntax match redifFieldInBook /^In-Book:/ skipwhite skipempty nextgroup=redifArgumentInBook contained
syntax match redifFieldISBN /^ISBN:/ skipwhite skipempty nextgroup=redifArgumentISBN contained
syntax match redifFieldISSN /^ISSN:/ skipwhite skipempty nextgroup=redifArgumentISSN contained
syntax match redifFieldIssue /^Issue:/ skipwhite skipempty nextgroup=redifArgumentIssue contained
syntax match redifFieldJournal /^Journal:/ skipwhite skipempty nextgroup=redifArgumentJournal contained
syntax match redifFieldKeywords /^Keywords:/ skipwhite skipempty nextgroup=redifArgumentKeywords contained
syntax match redifFieldKeywords /^Keywords:/ skipwhite skipempty nextgroup=redifArgumentKeywords contained
syntax match redifFieldLanguage /^Language:/ skipwhite skipempty nextgroup=redifArgumentLanguage contained
syntax match redifFieldLastLoginDate /^Last-Login-Date:/ skipwhite skipempty nextgroup=redifArgumentLastLoginDate contained
syntax match redifFieldLength /^Length:/ skipwhite skipempty nextgroup=redifArgumentLength contained
syntax match redifFieldMaintainerEmail /^Maintainer-Email:/ skipwhite skipempty nextgroup=redifArgumentMaintainerEmail contained
syntax match redifFieldMaintainerFax /^Maintainer-Fax:/ skipwhite skipempty nextgroup=redifArgumentMaintainerFax contained
syntax match redifFieldMaintainerName /^Maintainer-Name:/ skipwhite skipempty nextgroup=redifArgumentMaintainerName contained
syntax match redifFieldMaintainerPhone /^Maintainer-Phone:/ skipwhite skipempty nextgroup=redifArgumentMaintainerPhone contained
syntax match redifFieldMonth /^Month:/ skipwhite skipempty nextgroup=redifArgumentMonth contained
syntax match redifFieldNameASCII /^Name-ASCII:/ skipwhite skipempty nextgroup=redifArgumentNameASCII contained
syntax match redifFieldNameFirst /^Name-First:/ skipwhite skipempty nextgroup=redifArgumentNameFirst contained
syntax match redifFieldNameFull /^Name-Full:/ skipwhite skipempty nextgroup=redifArgumentNameFull contained
syntax match redifFieldNameLast /^Name-Last:/ skipwhite skipempty nextgroup=redifArgumentNameLast contained
syntax match redifFieldNameMiddle /^Name-Middle:/ skipwhite skipempty nextgroup=redifArgumentNameMiddle contained
syntax match redifFieldNamePrefix /^Name-Prefix:/ skipwhite skipempty nextgroup=redifArgumentNamePrefix contained
syntax match redifFieldNameSuffix /^Name-Suffix:/ skipwhite skipempty nextgroup=redifArgumentNameSuffix contained
syntax match redifFieldName /^Name:/ skipwhite skipempty nextgroup=redifArgumentName contained
syntax match redifFieldNote /^Note:/ skipwhite skipempty nextgroup=redifArgumentNote contained
syntax match redifFieldNotification /^Notification:/ skipwhite skipempty nextgroup=redifArgumentNotification contained
syntax match redifFieldNumber /^Number:/ skipwhite skipempty nextgroup=redifArgumentNumber contained
syntax match redifFieldOrderEmail /^Order-Email:/ skipwhite skipempty nextgroup=redifArgumentOrderEmail contained
syntax match redifFieldOrderHomepage /^Order-Homepage:/ skipwhite skipempty nextgroup=redifArgumentOrderHomepage contained
syntax match redifFieldOrderPostal /^Order-Postal:/ skipwhite skipempty nextgroup=redifArgumentOrderPostal contained
syntax match redifFieldOrderURL /^Order-URL:/ skipwhite skipempty nextgroup=redifArgumentOrderURL contained
syntax match redifFieldPages /^Pages:/ skipwhite skipempty nextgroup=redifArgumentPages contained
syntax match redifFieldPaperHandle /^Paper-Handle:/ skipwhite skipempty nextgroup=redifArgumentPaperHandle contained
syntax match redifFieldPhone /^Phone:/ skipwhite skipempty nextgroup=redifArgumentPhone contained
syntax match redifFieldPostal /^Postal:/ skipwhite skipempty nextgroup=redifArgumentPostal contained
syntax match redifFieldPredecessor /^Predecessor:/ skipwhite skipempty nextgroup=redifArgumentPredecessor contained
syntax match redifFieldPrice /^Price:/ skipwhite skipempty nextgroup=redifArgumentPrice contained
syntax match redifFieldPrimaryDefunct /^Primary-Defunct:/ skipwhite skipempty nextgroup=redifArgumentPrimaryDefunct contained
syntax match redifFieldPrimaryEmail /^Primary-Email:/ skipwhite skipempty nextgroup=redifArgumentPrimaryEmail contained
syntax match redifFieldPrimaryFax /^Primary-Fax:/ skipwhite skipempty nextgroup=redifArgumentPrimaryFax contained
syntax match redifFieldPrimaryHomepage /^Primary-Homepage:/ skipwhite skipempty nextgroup=redifArgumentPrimaryHomepage contained
syntax match redifFieldPrimaryInstitution /^Primary-Institution:/ skipwhite skipempty nextgroup=redifArgumentPrimaryInstitution contained
syntax match redifFieldPrimaryLocation /^Primary-Location:/ skipwhite skipempty nextgroup=redifArgumentPrimaryLocation contained
syntax match redifFieldPrimaryName /^Primary-Name:/ skipwhite skipempty nextgroup=redifArgumentPrimaryName contained
syntax match redifFieldPrimaryNameEnglish /^Primary-Name-English:/ skipwhite skipempty nextgroup=redifArgumentPrimaryNameEnglish contained
syntax match redifFieldPrimaryPhone /^Primary-Phone:/ skipwhite skipempty nextgroup=redifArgumentPrimaryPhone contained
syntax match redifFieldPrimaryPostal /^Primary-Postal:/ skipwhite skipempty nextgroup=redifArgumentPrimaryPostal contained
syntax match redifFieldProgrammingLanguage /^Programming-Language:/ skipwhite skipempty nextgroup=redifArgumentProgrammingLanguage contained
syntax match redifFieldProviderEmail /^Provider-Email:/ skipwhite skipempty nextgroup=redifArgumentProviderEmail contained
syntax match redifFieldProviderFax /^Provider-Fax:/ skipwhite skipempty nextgroup=redifArgumentProviderFax contained
syntax match redifFieldProviderHomepage /^Provider-Homepage:/ skipwhite skipempty nextgroup=redifArgumentProviderHomepage contained
syntax match redifFieldProviderInstitution /^Provider-Institution:/ skipwhite skipempty nextgroup=redifArgumentProviderInstitution contained
syntax match redifFieldProviderLocation /^Provider-Location:/ skipwhite skipempty nextgroup=redifArgumentProviderLocation contained
syntax match redifFieldProviderName /^Provider-Name:/ skipwhite skipempty nextgroup=redifArgumentProviderName contained
syntax match redifFieldProviderNameEnglish /^Provider-Name-English:/ skipwhite skipempty nextgroup=redifArgumentProviderNameEnglish contained
syntax match redifFieldProviderPhone /^Provider-Phone:/ skipwhite skipempty nextgroup=redifArgumentProviderPhone contained
syntax match redifFieldProviderPostal /^Provider-Postal:/ skipwhite skipempty nextgroup=redifArgumentProviderPostal contained
syntax match redifFieldPublicationDate /^Publication-Date:/ skipwhite skipempty nextgroup=redifArgumentPublicationDate contained
syntax match redifFieldPublicationStatus /^Publication-Status:/ skipwhite skipempty nextgroup=redifArgumentPublicationStatus contained
syntax match redifFieldPublicationType /^Publication-Type:/ skipwhite skipempty nextgroup=redifArgumentPublicationType contained
syntax match redifFieldPublisherEmail /^Publisher-Email:/ skipwhite skipempty nextgroup=redifArgumentPublisherEmail contained
syntax match redifFieldPublisherFax /^Publisher-Fax:/ skipwhite skipempty nextgroup=redifArgumentPublisherFax contained
syntax match redifFieldPublisherHomepage /^Publisher-Homepage:/ skipwhite skipempty nextgroup=redifArgumentPublisherHomepage contained
syntax match redifFieldPublisherInstitution /^Publisher-Institution:/ skipwhite skipempty nextgroup=redifArgumentPublisherInstitution contained
syntax match redifFieldPublisherLocation /^Publisher-Location:/ skipwhite skipempty nextgroup=redifArgumentPublisherLocation contained
syntax match redifFieldPublisherName /^Publisher-Name:/ skipwhite skipempty nextgroup=redifArgumentPublisherName contained
syntax match redifFieldPublisherNameEnglish /^Publisher-Name-English:/ skipwhite skipempty nextgroup=redifArgumentPublisherNameEnglish contained
syntax match redifFieldPublisherPhone /^Publisher-Phone:/ skipwhite skipempty nextgroup=redifArgumentPublisherPhone contained
syntax match redifFieldPublisherPostal /^Publisher-Postal:/ skipwhite skipempty nextgroup=redifArgumentPublisherPostal contained
syntax match redifFieldQuaternaryEmail /^Quaternary-Email:/ skipwhite skipempty nextgroup=redifArgumentQuaternaryEmail contained
syntax match redifFieldQuaternaryFax /^Quaternary-Fax:/ skipwhite skipempty nextgroup=redifArgumentQuaternaryFax contained
syntax match redifFieldQuaternaryHomepage /^Quaternary-Homepage:/ skipwhite skipempty nextgroup=redifArgumentQuaternaryHomepage contained
syntax match redifFieldQuaternaryInstitution /^Quaternary-Institution:/ skipwhite skipempty nextgroup=redifArgumentQuaternaryInstitution contained
syntax match redifFieldQuaternaryLocation /^Quaternary-Location:/ skipwhite skipempty nextgroup=redifArgumentQuaternaryLocation contained
syntax match redifFieldQuaternaryName /^Quaternary-Name:/ skipwhite skipempty nextgroup=redifArgumentQuaternaryName contained
syntax match redifFieldQuaternaryNameEnglish /^Quaternary-Name-English:/ skipwhite skipempty nextgroup=redifArgumentQuaternaryNameEnglish contained
syntax match redifFieldQuaternaryPhone /^Quaternary-Phone:/ skipwhite skipempty nextgroup=redifArgumentQuaternaryPhone contained
syntax match redifFieldQuaternaryPostal /^Quaternary-Postal:/ skipwhite skipempty nextgroup=redifArgumentQuaternaryPostal contained
syntax match redifFieldRegisteredDate /^Registered-Date:/ skipwhite skipempty nextgroup=redifArgumentRegisteredDate contained
syntax match redifFieldRequires /^Requires:/ skipwhite skipempty nextgroup=redifArgumentRequires contained
syntax match redifFieldRestriction /^Restriction:/ skipwhite skipempty nextgroup=redifArgumentRestriction contained
syntax match redifFieldRevisionDate /^Revision-Date:/ skipwhite skipempty nextgroup=redifArgumentRevisionDate contained
syntax match redifFieldSecondaryDefunct /^Secondary-Defunct:/ skipwhite skipempty nextgroup=redifArgumentSecondaryDefunct contained
syntax match redifFieldSecondaryEmail /^Secondary-Email:/ skipwhite skipempty nextgroup=redifArgumentSecondaryEmail contained
syntax match redifFieldSecondaryFax /^Secondary-Fax:/ skipwhite skipempty nextgroup=redifArgumentSecondaryFax contained
syntax match redifFieldSecondaryHomepage /^Secondary-Homepage:/ skipwhite skipempty nextgroup=redifArgumentSecondaryHomepage contained
syntax match redifFieldSecondaryInstitution /^Secondary-Institution:/ skipwhite skipempty nextgroup=redifArgumentSecondaryInstitution contained
syntax match redifFieldSecondaryLocation /^Secondary-Location:/ skipwhite skipempty nextgroup=redifArgumentSecondaryLocation contained
syntax match redifFieldSecondaryName /^Secondary-Name:/ skipwhite skipempty nextgroup=redifArgumentSecondaryName contained
syntax match redifFieldSecondaryNameEnglish /^Secondary-Name-English:/ skipwhite skipempty nextgroup=redifArgumentSecondaryNameEnglish contained
syntax match redifFieldSecondaryPhone /^Secondary-Phone:/ skipwhite skipempty nextgroup=redifArgumentSecondaryPhone contained
syntax match redifFieldSecondaryPostal /^Secondary-Postal:/ skipwhite skipempty nextgroup=redifArgumentSecondaryPostal contained
syntax match redifFieldSeries /^Series:/ skipwhite skipempty nextgroup=redifArgumentSeries contained
syntax match redifFieldShortId /^Short-Id:/ skipwhite skipempty nextgroup=redifArgumentShortId contained
syntax match redifFieldSize /^Size:/ skipwhite skipempty nextgroup=redifArgumentSize contained
syntax match redifFieldSoftwareHandle /^Software-Handle:/ skipwhite skipempty nextgroup=redifArgumentSoftwareHandle contained
syntax match redifFieldTemplateType /^Template-Type:/ skipwhite skipempty nextgroup=redifArgumentTemplateType contained
syntax match redifFieldTertiaryDefunct /^Tertiary-Defunct:/ skipwhite skipempty nextgroup=redifArgumentTertiaryDefunct contained
syntax match redifFieldTertiaryEmail /^Tertiary-Email:/ skipwhite skipempty nextgroup=redifArgumentTertiaryEmail contained
syntax match redifFieldTertiaryFax /^Tertiary-Fax:/ skipwhite skipempty nextgroup=redifArgumentTertiaryFax contained
syntax match redifFieldTertiaryHomepage /^Tertiary-Homepage:/ skipwhite skipempty nextgroup=redifArgumentTertiaryHomepage contained
syntax match redifFieldTertiaryInstitution /^Tertiary-Institution:/ skipwhite skipempty nextgroup=redifArgumentTertiaryInstitution contained
syntax match redifFieldTertiaryLocation /^Tertiary-Location:/ skipwhite skipempty nextgroup=redifArgumentTertiaryLocation contained
syntax match redifFieldTertiaryName /^Tertiary-Name:/ skipwhite skipempty nextgroup=redifArgumentTertiaryName contained
syntax match redifFieldTertiaryNameEnglish /^Tertiary-Name-English:/ skipwhite skipempty nextgroup=redifArgumentTertiaryNameEnglish contained
syntax match redifFieldTertiaryPhone /^Tertiary-Phone:/ skipwhite skipempty nextgroup=redifArgumentTertiaryPhone contained
syntax match redifFieldTertiaryPostal /^Tertiary-Postal:/ skipwhite skipempty nextgroup=redifArgumentTertiaryPostal contained
syntax match redifFieldTitle /^Title:/ skipwhite skipempty nextgroup=redifArgumentTitle contained
syntax match redifFieldType /^Type:/ skipwhite skipempty nextgroup=redifArgumentType contained
syntax match redifFieldURL /^URL:/ skipwhite skipempty nextgroup=redifArgumentURL contained
syntax match redifFieldVersion /^Version:/ skipwhite skipempty nextgroup=redifArgumentVersion contained
syntax match redifFieldVolume /^Volume:/ skipwhite skipempty nextgroup=redifArgumentVolume contained
syntax match redifFieldWorkplaceEmail /^Workplace-Email:/ skipwhite skipempty nextgroup=redifArgumentWorkplaceEmail contained
syntax match redifFieldWorkplaceFax /^Workplace-Fax:/ skipwhite skipempty nextgroup=redifArgumentWorkplaceFax contained
syntax match redifFieldWorkplaceHomepage /^Workplace-Homepage:/ skipwhite skipempty nextgroup=redifArgumentWorkplaceHomepage contained
syntax match redifFieldWorkplaceInstitution /^Workplace-Institution:/ skipwhite skipempty nextgroup=redifArgumentWorkplaceInstitution contained
syntax match redifFieldWorkplaceLocation /^Workplace-Location:/ skipwhite skipempty nextgroup=redifArgumentWorkplaceLocation contained
syntax match redifFieldWorkplaceName /^Workplace-Name:/ skipwhite skipempty nextgroup=redifArgumentWorkplaceName contained
syntax match redifFieldWorkplaceNameEnglish /^Workplace-Name-English:/ skipwhite skipempty nextgroup=redifArgumentWorkplaceNameEnglish contained
syntax match redifFieldWorkplaceOrganization /^Workplace-Organization:/ skipwhite skipempty nextgroup=redifArgumentWorkplaceOrganization contained
syntax match redifFieldWorkplacePhone /^Workplace-Phone:/ skipwhite skipempty nextgroup=redifArgumentWorkplacePhone contained
syntax match redifFieldWorkplacePostal /^Workplace-Postal:/ skipwhite skipempty nextgroup=redifArgumentWorkplacePostal contained
syntax match redifFieldYear /^Year:/ skipwhite skipempty nextgroup=redifArgumentYear contained

highlight def link redifFieldAbstract redifField
highlight def link redifFieldArticleHandle redifField
highlight def link redifFieldAuthorArticle redifField
highlight def link redifFieldAuthorBook redifField
highlight def link redifFieldAuthorChapter redifField
highlight def link redifFieldAuthorEmail redifField
highlight def link redifFieldAuthorFax redifField
highlight def link redifFieldAuthorHomepage redifField
highlight def link redifFieldAuthorName redifField
highlight def link redifFieldAuthorNameFirst redifField
highlight def link redifFieldAuthorNameLast redifField
highlight def link redifFieldAuthorPaper redifField
highlight def link redifFieldAuthorPerson redifField
highlight def link redifFieldAuthorPhone redifField
highlight def link redifFieldAuthorPostal redifField
highlight def link redifFieldAuthorSoftware redifField
highlight def link redifFieldAuthorWorkplaceEmail redifField
highlight def link redifFieldAuthorWorkplaceFax redifField
highlight def link redifFieldAuthorWorkplaceHomepage redifField
highlight def link redifFieldAuthorWorkplaceInstitution redifField
highlight def link redifFieldAuthorWorkplaceLocation redifField
highlight def link redifFieldAuthorWorkplaceName redifField
highlight def link redifFieldAuthorWorkplaceNameEnglish redifField
highlight def link redifFieldAuthorWorkplacePhone redifField
highlight def link redifFieldAuthorWorkplacePostal redifField
highlight def link redifFieldAvailability redifField
highlight def link redifFieldBookHandle redifField
highlight def link redifFieldBookTitle redifField
highlight def link redifFieldChapterHandle redifField
highlight def link redifFieldChapter redifField
highlight def link redifFieldClassificationJEL redifField
highlight def link redifFieldContactEmail redifField
highlight def link redifFieldCreationDate redifField
highlight def link redifFieldDescription redifField
highlight def link redifFieldEdition redifField
highlight def link redifFieldEditorBook redifField
highlight def link redifFieldEditorEmail redifField
highlight def link redifFieldEditorFax redifField
highlight def link redifFieldEditorHomepage redifField
highlight def link redifFieldEditorName redifField
highlight def link redifFieldEditorNameFirst redifField
highlight def link redifFieldEditorNameLast redifField
highlight def link redifFieldEditorPerson redifField
highlight def link redifFieldEditorPhone redifField
highlight def link redifFieldEditorPostal redifField
highlight def link redifFieldEditorSeries redifField
highlight def link redifFieldEditorWorkplaceEmail redifField
highlight def link redifFieldEditorWorkplaceFax redifField
highlight def link redifFieldEditorWorkplaceHomepage redifField
highlight def link redifFieldEditorWorkplaceInstitution redifField
highlight def link redifFieldEditorWorkplaceLocation redifField
highlight def link redifFieldEditorWorkplaceName redifField
highlight def link redifFieldEditorWorkplaceNameEnglish redifField
highlight def link redifFieldEditorWorkplacePhone redifField
highlight def link redifFieldEditorWorkplacePostal redifField
highlight def link redifFieldEmail redifField
highlight def link redifFieldFax redifField
highlight def link redifFieldFileFormat redifField
highlight def link redifFieldFileFunction redifField
highlight def link redifFieldFileRestriction redifField
highlight def link redifFieldFileSize redifField
highlight def link redifFieldFileURL redifField
highlight def link redifFieldFollowup redifField
highlight def link redifFieldHandleOfArchive redifField
highlight def link redifFieldHandleOfInstitution redifField
highlight def link redifFieldHandleOfPerson redifField
highlight def link redifFieldHandleOfSeries redifField
highlight def link redifFieldHandleOfWork redifField
highlight def link redifFieldHasChapter redifField
highlight def link redifFieldHomepage redifField
highlight def link redifFieldInBook redifField
highlight def link redifFieldISBN redifField
highlight def link redifFieldISSN redifField
highlight def link redifFieldIssue redifField
highlight def link redifFieldJournal redifField
highlight def link redifFieldKeywords redifField
highlight def link redifFieldKeywords redifField
highlight def link redifFieldLanguage redifField
highlight def link redifFieldLastLoginDate redifField
highlight def link redifFieldLength redifField
highlight def link redifFieldMaintainerEmail redifField
highlight def link redifFieldMaintainerFax redifField
highlight def link redifFieldMaintainerName redifField
highlight def link redifFieldMaintainerPhone redifField
highlight def link redifFieldMonth redifField
highlight def link redifFieldNameASCII redifField
highlight def link redifFieldNameFirst redifField
highlight def link redifFieldNameFull redifField
highlight def link redifFieldNameLast redifField
highlight def link redifFieldNameMiddle redifField
highlight def link redifFieldNamePrefix redifField
highlight def link redifFieldNameSuffix redifField
highlight def link redifFieldName redifField
highlight def link redifFieldNote redifField
highlight def link redifFieldNotification redifField
highlight def link redifFieldNumber redifField
highlight def link redifFieldOrderEmail redifField
highlight def link redifFieldOrderHomepage redifField
highlight def link redifFieldOrderPostal redifField
highlight def link redifFieldOrderURL redifField
highlight def link redifFieldPages redifField
highlight def link redifFieldPaperHandle redifField
highlight def link redifFieldPhone redifField
highlight def link redifFieldPostal redifField
highlight def link redifFieldPredecessor redifField
highlight def link redifFieldPrice redifField
highlight def link redifFieldPrimaryDefunct redifField
highlight def link redifFieldPrimaryEmail redifField
highlight def link redifFieldPrimaryFax redifField
highlight def link redifFieldPrimaryHomepage redifField
highlight def link redifFieldPrimaryInstitution redifField
highlight def link redifFieldPrimaryLocation redifField
highlight def link redifFieldPrimaryName redifField
highlight def link redifFieldPrimaryNameEnglish redifField
highlight def link redifFieldPrimaryPhone redifField
highlight def link redifFieldPrimaryPostal redifField
highlight def link redifFieldProgrammingLanguage redifField
highlight def link redifFieldProviderEmail redifField
highlight def link redifFieldProviderFax redifField
highlight def link redifFieldProviderHomepage redifField
highlight def link redifFieldProviderInstitution redifField
highlight def link redifFieldProviderLocation redifField
highlight def link redifFieldProviderName redifField
highlight def link redifFieldProviderNameEnglish redifField
highlight def link redifFieldProviderPhone redifField
highlight def link redifFieldProviderPostal redifField
highlight def link redifFieldPublicationDate redifField
highlight def link redifFieldPublicationStatus redifField
highlight def link redifFieldPublicationType redifField
highlight def link redifFieldPublisherEmail redifField
highlight def link redifFieldPublisherFax redifField
highlight def link redifFieldPublisherHomepage redifField
highlight def link redifFieldPublisherInstitution redifField
highlight def link redifFieldPublisherLocation redifField
highlight def link redifFieldPublisherName redifField
highlight def link redifFieldPublisherNameEnglish redifField
highlight def link redifFieldPublisherPhone redifField
highlight def link redifFieldPublisherPostal redifField
highlight def link redifFieldQuaternaryEmail redifField
highlight def link redifFieldQuaternaryFax redifField
highlight def link redifFieldQuaternaryHomepage redifField
highlight def link redifFieldQuaternaryInstitution redifField
highlight def link redifFieldQuaternaryLocation redifField
highlight def link redifFieldQuaternaryName redifField
highlight def link redifFieldQuaternaryNameEnglish redifField
highlight def link redifFieldQuaternaryPhone redifField
highlight def link redifFieldQuaternaryPostal redifField
highlight def link redifFieldRegisteredDate redifField
highlight def link redifFieldRequires redifField
highlight def link redifFieldRestriction redifField
highlight def link redifFieldRevisionDate redifField
highlight def link redifFieldSecondaryDefunct redifField
highlight def link redifFieldSecondaryEmail redifField
highlight def link redifFieldSecondaryFax redifField
highlight def link redifFieldSecondaryHomepage redifField
highlight def link redifFieldSecondaryInstitution redifField
highlight def link redifFieldSecondaryLocation redifField
highlight def link redifFieldSecondaryName redifField
highlight def link redifFieldSecondaryNameEnglish redifField
highlight def link redifFieldSecondaryPhone redifField
highlight def link redifFieldSecondaryPostal redifField
highlight def link redifFieldSeries redifField
highlight def link redifFieldShortId redifField
highlight def link redifFieldSize redifField
highlight def link redifFieldSoftwareHandle redifField
highlight def link redifFieldTemplateType redifField
highlight def link redifFieldTertiaryDefunct redifField
highlight def link redifFieldTertiaryEmail redifField
highlight def link redifFieldTertiaryFax redifField
highlight def link redifFieldTertiaryHomepage redifField
highlight def link redifFieldTertiaryInstitution redifField
highlight def link redifFieldTertiaryLocation redifField
highlight def link redifFieldTertiaryName redifField
highlight def link redifFieldTertiaryNameEnglish redifField
highlight def link redifFieldTertiaryPhone redifField
highlight def link redifFieldTertiaryPostal redifField
highlight def link redifFieldTitle redifField
highlight def link redifFieldTitle redifField
highlight def link redifFieldType redifField
highlight def link redifFieldURL redifField
highlight def link redifFieldVersion redifField
highlight def link redifFieldVolume redifField
highlight def link redifFieldWorkplaceEmail redifField
highlight def link redifFieldWorkplaceFax redifField
highlight def link redifFieldWorkplaceHomepage redifField
highlight def link redifFieldWorkplaceInstitution redifField
highlight def link redifFieldWorkplaceLocation redifField
highlight def link redifFieldWorkplaceName redifField
highlight def link redifFieldWorkplaceNameEnglish redifField
highlight def link redifFieldWorkplaceOrganization redifField
highlight def link redifFieldWorkplacePhone redifField
highlight def link redifFieldWorkplacePostal redifField
highlight def link redifFieldYear redifField

" Standard arguments
"    Contains all the remaining line if it is not a new field
"    /\%(^\S\{-}:\)\@!\S.*/
"    Note: Those arguments are not highlighted so far. They are here for
"    future extensions.
"    Note: Those matches do not extend further the end of the line. They are
"    unfit for arguments that may span several lines like Title, Abstract,
"    Postal. They are well-fit for arguments that must not span more than one
"    line by definition, such as URLs, Email addresses, etc.
"    TODO Find more RegEx for these arguments
"    	TODO Fax, Phone
"    	TODO URL, Homepage
"    	TODO Keywords
"    	TODO Classification-JEL
"    	TODO Short-Id, Author-Person, Editor-Person
"syntax match redifArgumentAuthorFax /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentAuthorHomepage /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentAuthorName /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentAuthorNameFirst /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentAuthorNameLast /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentAuthorPerson /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentAuthorPhone /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentAuthorPostal /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentAuthorWorkplaceFax /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentAuthorWorkplaceHomepage /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentAuthorWorkplaceLocation /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentAuthorWorkplaceName /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentAuthorWorkplaceNameEnglish /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentAuthorWorkplacePhone /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentAuthorWorkplacePostal /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentEditorFax /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentEditorHomepage /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentEditorName /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentEditorNameFirst /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentEditorNameLast /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentEditorPerson /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentEditorPhone /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentEditorPostal /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentEditorWorkplaceFax /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentEditorWorkplaceHomepage /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentEditorWorkplaceLocation /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentEditorWorkplaceName /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentEditorWorkplaceNameEnglish /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentEditorWorkplacePhone /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentEditorWorkplacePostal /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentFax /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentFileFunction /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentFileURL /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentHomepage /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentIssue /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentJournal /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentMaintainerFax /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentMaintainerName /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentMaintainerPhone /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentNameFirst /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentNameFull /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentNameLast /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentNameMiddle /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentNamePrefix /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentNameSuffix /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentName /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentOrderHomepage /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentOrderPostal /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentOrderURL /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPhone /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPostal /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPrice /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPrimaryFax /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPrimaryHomepage /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPrimaryLocation /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPrimaryName /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPrimaryNameEnglish /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPrimaryPhone /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPrimaryPostal /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentProviderFax /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentProviderHomepage /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentProviderLocation /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentProviderName /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentProviderNameEnglish /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentProviderPhone /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentProviderPostal /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPublisherFax /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPublisherHomepage /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPublisherLocation /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPublisherName /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPublisherNameEnglish /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPublisherPhone /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentPublisherPostal /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentQuaternaryFax /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentQuaternaryHomepage /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentQuaternaryLocation /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentQuaternaryName /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentQuaternaryNameEnglish /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentQuaternaryPhone /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentQuaternaryPostal /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentRequires /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentSecondaryFax /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentSecondaryHomepage /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentSecondaryLocation /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentSecondaryName /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentSecondaryNameEnglish /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentSecondaryPhone /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentSecondaryPostal /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentSeries /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentSize /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentShortId /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentTertiaryFax /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentTertiaryHomepage /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentTertiaryLocation /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentTertiaryName /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentTertiaryNameEnglish /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentTertiaryPhone /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentTertiaryPostal /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentURL /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentVersion /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentWorkplaceFax /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentWorkplaceHomepage /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentWorkplaceLocation /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentWorkplaceName /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentWorkplaceNameEnglish /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentWorkplaceOrganization /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentWorkplacePhone /\%(^\S\{-}:\)\@!\S.*/ contained display
"syntax match redifArgumentWorkplacePostal /\%(^\S\{-}:\)\@!\S.*/ contained display

" Special arguments
"    Those arguments require special values
"    TODO Improve some RegEx
"    	TODO Improve Emails
"    	TODO Improve ISBN
"    	TODO Improve ISSN
"    	TODO Improve spell check (add words from economics.
"    	   expl=macroeconometrics, Schumpeterian, IS-LM, etc.)
"
"    Template-Type
syntax match redifArgumentTemplateType /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectTemplateType contained display
syntax match redifCorrectTemplateType /ReDIF-\%(Paper\|Article\|Chapter\|Book\|Software\|Archive\|Series\|Institution\|Person\)/ nextgroup=redifTemplateVersionNumber contained display
syntax match redifTemplateVersionNumber / \d\+\.\d\+/ contained display

highlight def link redifArgumentTemplateType redifError
highlight def link redifCorrectTemplateType Constant
highlight def link redifTemplateVersionNumber Number

"    Handles:
"
"        Handles of Works:
syntax match redifArgumentHandleOfWork /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfWork contained display
syntax match redifArgumentAuthorArticle /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfWork contained display
syntax match redifArgumentAuthorBook /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfWork contained display
syntax match redifArgumentAuthorChapter /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfWork contained display
syntax match redifArgumentAuthorPaper /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfWork contained display
syntax match redifArgumentAuthorSoftware /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfWork contained display
syntax match redifArgumentEditorBook /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfWork contained display
syntax match redifArgumentEditorSeries /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfWork contained display
syntax match redifArgumentInBook /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfWork contained display
syntax match redifArgumentHasChapter /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfWork contained display
syntax match redifArgumentArticleHandle /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfWork contained display
syntax match redifArgumentBookHandle /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfWork contained display
syntax match redifArgumentChapterHandle /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfWork contained display
syntax match redifArgumentPaperHandle /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfWork contained display
syntax match redifArgumentSoftwareHandle /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfWork contained display
syntax match redifCorrectHandleOfWork /RePEc:\a\a\a:\%(_\@!\w\)\{6}:\S\+/ contains=redifForbiddenCharactersInHandle,redifBestPracticeInHandle contained display
" TODO Are those characters really forbidden???
syntax match redifForbiddenCharactersInHandle /[\/*?"<>|]/ contained display
syntax match redifBestPracticeInHandle /\<\%([vi]:[1-9]\d*\|y:[1-9]\d\{3}\|p:[1-9]\d*-[1-9]\d*\|i:\%(jan\|feb\|mar\|apr\|may\|jun\|jul\|aug\|sep\|oct\|nov\|dec\|spr\|sum\|aut\|win\|spe\|Q[1-4]\|\d\d-\d\d\)\|Q:[1-4]\)\>/ contained display

highlight def link redifArgumentHandleOfWork redifError
highlight def link redifArgumentAuthorArticle redifError
highlight def link redifArgumentAuthorBook redifError
highlight def link redifArgumentAuthorChapter redifError
highlight def link redifArgumentAuthorPaper redifError
highlight def link redifArgumentAuthorSoftware redifError
highlight def link redifArgumentEditorBook redifError
highlight def link redifArgumentEditorSeries redifError
highlight def link redifArgumentInBook redifError
highlight def link redifArgumentHasChapter redifError
highlight def link redifArgumentArticleHandle redifError
highlight def link redifArgumentBookHandle redifError
highlight def link redifArgumentChapterHandle redifError
highlight def link redifArgumentPaperHandle redifError
highlight def link redifArgumentSoftwareHandle redifError
highlight def link redifForbiddenCharactersInHandle redifError
highlight def link redifBestPracticeInHandle redifSpecial

"        Handles of Series:
syntax match redifArgumentHandleOfSeries /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfSeries contained display
syntax match redifArgumentFollowup /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfSeries contained display
syntax match redifArgumentPredecessor /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfSeries contained display
syntax match redifCorrectHandleOfSeries /RePEc:\a\a\a:\%(_\@!\w\)\{6}/ contained display

highlight def link redifArgumentHandleOfSeries redifError
highlight def link redifArgumentFollowup redifError
highlight def link redifArgumentPredecessor redifError

"        Handles of Archives:
syntax match redifArgumentHandleOfArchive /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfArchive contained display
syntax match redifCorrectHandleOfArchive /RePEc:\a\a\a/ contained display

highlight def link redifArgumentHandleOfArchive redifError

"        Handles of Person:
syntax match redifArgumentHandleOfPerson /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfPerson contained display
syntax match redifCorrectHandleOfPerson /\%(\%(:\@!\S\)\{-}:\)\{2}[1-9]\d\{3}\%(-02\%(-[12]\d\|-0[1-9]\)\|-\%(0[469]\|11\)\%(-30\|-[12]\d\|-0[1-9]\)\|-\%(0[13578]\|1[02]\)\%(-3[01]\|-[12]\d\|-0[1-9]\)\):\S\+/ contained display

highlight def link redifArgumentHandleOfPerson redifError

"        Handles of Institution:
syntax match redifArgumentAuthorWorkplaceInstitution /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfInstitution contained display
syntax match redifArgumentEditorWorkplaceInstitution /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfInstitution contained display
syntax match redifArgumentPrimaryInstitution /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfInstitution contained display
syntax match redifArgumentProviderInstitution /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfInstitution contained display
syntax match redifArgumentPublisherInstitution /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfInstitution contained display
syntax match redifArgumentQuaternaryInstitution /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfInstitution contained display
syntax match redifArgumentSecondaryInstitution /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfInstitution contained display
syntax match redifArgumentTertiaryInstitution /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfInstitution contained display
syntax match redifArgumentWorkplaceInstitution /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfInstitution contained display
syntax match redifArgumentHandleOfInstitution /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfInstitution contained display
syntax match redifArgumentPrimaryDefunct /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfInstitution contained display
syntax match redifArgumentSecondaryDefunct /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfInstitution contained display
syntax match redifArgumentTertiaryDefunct /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectHandleOfInstitution contained display
" TODO Are digits authorized? Apparently not.
" Country codes:
" http://www.iso.org/iso/country_codes/iso_3166_code_lists/country_names_and_code_elements.htm
syntax match redifCorrectHandleOfInstitution /RePEc:\a\a\a:\a\{5}\(ea\|af\|ax\|al\|dz\|as\|ad\|ao\|ai\|aq\|ag\|ar\|am\|aw\|au\|at\|az\|bs\|bh\|bd\|bb\|by\|be\|bz\|bj\|bm\|bt\|bo\|bq\|ba\|bw\|bv\|br\|io\|bn\|bg\|bf\|bi\|kh\|cm\|ca\|cv\|ky\|cf\|td\|cl\|cn\|cx\|cc\|co\|km\|cg\|cd\|ck\|cr\|ci\|hr\|cu\|cw\|cy\|cz\|dk\|dj\|dm\|do\|ec\|eg\|sv\|gq\|er\|ee\|et\|fk\|fo\|fj\|fi\|fr\|gf\|pf\|tf\|ga\|gm\|ge\|de\|gh\|gi\|gr\|gl\|gd\|gp\|gu\|gt\|gg\|gn\|gw\|gy\|ht\|hm\|va\|hn\|hk\|hu\|is\|in\|id\|ir\|iq\|ie\|im\|il\|it\|jm\|jp\|je\|jo\|kz\|ke\|ki\|kp\|kr\|kw\|kg\|la\|lv\|lb\|ls\|lr\|ly\|li\|lt\|lu\|mo\|mk\|mg\|mw\|my\|mv\|ml\|mt\|mh\|mq\|mr\|mu\|yt\|mx\|fm\|md\|mc\|mn\|me\|ms\|ma\|mz\|mm\|na\|nr\|np\|nl\|nc\|nz\|ni\|ne\|ng\|nu\|nf\|mp\|no\|om\|pk\|pw\|ps\|pa\|pg\|py\|pe\|ph\|pn\|pl\|pt\|pr\|qa\|re\|ro\|ru\|rw\|bl\|sh\|kn\|lc\|mf\|pm\|vc\|ws\|sm\|st\|sa\|sn\|rs\|sc\|sl\|sg\|sx\|sk\|si\|sb\|so\|za\|gs\|ss\|es\|lk\|sd\|sr\|sj\|sz\|se\|ch\|sy\|tw\|tj\|tz\|th\|tl\|tg\|tk\|to\|tt\|tn\|tr\|tm\|tc\|tv\|ug\|ua\|ae\|gb\|us\|um\|uy\|uz\|vu\|ve\|vn\|vg\|vi\|wf\|eh\|ye\|zm\|zw\)/ contained display

highlight def link redifArgumentHandleOfInstitution redifError
highlight def link redifArgumentPrimaryDefunct redifError
highlight def link redifArgumentSecondaryDefunct redifError
highlight def link redifArgumentTertiaryDefunct redifError

"    Emails:
syntax match redifArgumentAuthorEmail /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectEmail contained display
syntax match redifArgumentAuthorWorkplaceEmail /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectEmail contained display
syntax match redifArgumentContactEmail /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectEmail contained display
syntax match redifArgumentEditorEmail /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectEmail contained display
syntax match redifArgumentEditorWorkplaceEmail /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectEmail contained display
syntax match redifArgumentEmail /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectEmail contained display
syntax match redifArgumentMaintainerEmail /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectEmail contained display
syntax match redifArgumentOrderEmail /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectEmail contained display
syntax match redifArgumentPrimaryEmail /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectEmail contained display
syntax match redifArgumentProviderEmail /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectEmail contained display
syntax match redifArgumentPublisherEmail /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectEmail contained display
syntax match redifArgumentQuaternaryEmail /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectEmail contained display
syntax match redifArgumentSecondaryEmail /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectEmail contained display
syntax match redifArgumentTertiaryEmail /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectEmail contained display
syntax match redifArgumentWorkplaceEmail /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectEmail contained display
syntax match redifCorrectEmail /\%(@\@!\S\)\+@\%(@\@!\S\)\+/ contained display

highlight def link redifArgumentAuthorEmail redifError
highlight def link redifArgumentAuthorWorkplaceEmail redifError
highlight def link redifArgumentContactEmail redifError
highlight def link redifArgumentEditorEmail redifError
highlight def link redifArgumentEditorWorkplaceEmail redifError
highlight def link redifArgumentEmail redifError
highlight def link redifArgumentMaintainerEmail redifError
highlight def link redifArgumentOrderEmail redifError
highlight def link redifArgumentPrimaryEmail redifError
highlight def link redifArgumentProviderEmail redifError
highlight def link redifArgumentPublisherEmail redifError
highlight def link redifArgumentQuaternaryEmail redifError
highlight def link redifArgumentSecondaryEmail redifError
highlight def link redifArgumentTertiaryEmail redifError
highlight def link redifArgumentWorkplaceEmail redifError

"    Language
"    Source: https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes
syntax match redifArgumentLanguage /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectLanguage contained display
syntax match redifCorrectLanguage /\<\(aa\|ab\|af\|ak\|als\|am\|an\|ang\|ar\|arc\|as\|ast\|av\|ay\|az\|ba\|bar\|bat-smg\|bcl\|be\|be-x-old\|bg\|bh\|bi\|bm\|bn\|bo\|bpy\|br\|bs\|bug\|bxr\|ca\|ce\|ceb\|ch\|cho\|chr\|chy\|co\|cr\|cs\|csb\|cu\|cv\|cy\|da\|de\|diq\|dsb\|dv\|dz\|ee\|el\|en\|eo\|es\|et\|eu\|ext\|fa\|ff\|fi\|fiu-vro\|fj\|fo\|fr\|frp\|fur\|fy\|ga\|gd\|gil\|gl\|gn\|got\|gu\|gv\|ha\|haw\|he\|hi\|ho\|hr\|ht\|hu\|hy\|hz\|ia\|id\|ie\|ig\|ii\|ik\|ilo\|io\|is\|it\|iu\|ja\|jbo\|jv\|ka\|kg\|ki\|kj\|kk\|kl\|km\|kn\|khw\|ko\|kr\|ks\|ksh\|ku\|kv\|kw\|ky\|la\|lad\|lan\|lb\|lg\|li\|lij\|lmo\|ln\|lo\|lt\|lv\|map-bms\|mg\|mh\|mi\|mk\|ml\|mn\|mo\|mr\|ms\|mt\|mus\|my\|na\|nah\|nap\|nd\|nds\|nds-nl\|ne\|new\|ng\|nl\|nn\|no\|nr\|nso\|nrm\|nv\|ny\|oc\|oj\|om\|or\|os\|pa\|pag\|pam\|pap\|pdc\|pi\|pih\|pl\|pms\|ps\|pt\|qu\|rm\|rmy\|rn\|ro\|roa-rup\|ru\|rw\|sa\|sc\|scn\|sco\|sd\|se\|sg\|sh\|si\|simple\|sk\|sl\|sm\|sn\|so\|sq\|sr\|ss\|st\|su\|sv\|sw\|ta\|te\|tet\|tg\|th\|ti\|tk\|tl\|tlh\|tn\|to\|tpi\|tr\|ts\|tt\|tum\|tw\|ty\|udm\|ug\|uk\|ur\|uz\|ve\|vi\|vec\|vls\|vo\|wa\|war\|wo\|xal\|xh\|yi\|yo\|za\|zh\|zh-min-nan\|zh-yue\|zu\)\>/ contained display

highlight def link redifArgumentLanguage redifError
highlight def link redifCorrectLanguage redifSpecial

"    Length
"    Based on the example in the documentation. But apparently any field is
"    possible
syntax match redifArgumentLength /\%(^\S\{-}:\)\@!\S.*/ contains=redifGoodLength contained display
syntax match redifGoodLength /1 page\|[1-9]\d*\%( pages\)\=/ contained display

highlight def link redifGoodLength redifSpecial

"    Publication-Type
syntax match redifArgumentPublicationType /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectPublicationType contained display
syntax match redifCorrectPublicationType /\<\(journal article\|book\|book chapter\|working paper\|conference paper\|report\|other\)\>/ contained display

highlight def link redifArgumentPublicationType redifError
highlight def link redifCorrectPublicationType redifSpecial

"    Publication-Status
syntax match redifArgumentPublicationStatus /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectPublicationStatus contained display
syntax match redifCorrectPublicationStatus /\<\%(published\|forthcoming\)\>.*/ contains=redifSpecialPublicationStatus contained display
syntax match redifSpecialPublicationStatus /published\|forthcoming/ contained display

highlight def link redifArgumentPublicationStatus redifError
highlight def link redifSpecialPublicationStatus redifSpecial

"    Month
"    TODO Are numbers also allowed?
syntax match redifArgumentMonth /\%(^\S\{-}:\)\@!\S.*/ contains=redifGoodMonth contained display
syntax match redifGoodMonth /\<\(Jan\%(uary\)\=\|Feb\%(ruary\)\=\|Mar\%(ch\)\=\|Apr\%(il\)\=\|May\|June\=\|July\=\|Aug\%(ust\)\=\|Sep\%(tember\)\=\|Oct\%(ober\)\=\|Nov\%(ember\)\=\|Dec\%(ember\)\=\)\>/ contained display

highlight def link redifGoodMonth redifSpecial

"    Integers: Number, Volume, Chapter
syntax match redifArgumentNumber /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectInteger contained display
syntax match redifArgumentVolume /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectInteger contained display
syntax match redifArgumentChapter /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectInteger contained display
syntax match redifCorrectInteger /[1-9]\d*/ contained display

highlight def link redifArgumentVolume redifError
highlight def link redifArgumentChapter redifError

"    Year
syntax match redifArgumentYear /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectYear contained display
syntax match redifCorrectYear /[1-9]\d\{3}/ contained display

highlight def link redifArgumentYear redifError

"    Edition
"    Based on the example in the documentation.
syntax match redifArgumentEdition /\%(^\S\{-}:\)\@!\S.*/ contains=redifGoodEdition contained display
syntax match redifGoodEdition /1st\|2nd\|3rd\|[4-9]th\|[1-9]\d*\%(1st\|2nd\|3rd\|[4-9]th\)\|[1-9]\d*/ contained display

highlight def link redifGoodEdition redifSpecial

"    ISBN
syntax match redifArgumentISBN /\%(^\S\{-}:\)\@!\S.*/ contains=redifGoodISBN contained display
syntax match redifGoodISBN /\d[0-9-]\{8,15}\d/ contained display

highlight def link redifGoodISBN redifSpecial

"    ISSN
syntax match redifArgumentISSN /\%(^\S\{-}:\)\@!\S.*/ contains=redifGoodISSN contained display
syntax match redifGoodISSN /\d\{4}-\d\{3}[0-9X]/ contained display

highlight def link redifGoodISSN redifSpecial

"    File-Size
"    Based on the example in the documentation.
syntax match redifArgumentFileSize /\%(^\S\{-}:\)\@!\S.*/ contains=redifGoodSize contained display
syntax match redifGoodSize /kb\|bytes/ contained display

highlight def link redifGoodSize redifSpecial

"    Type
syntax match redifArgumentType /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectType contained display
syntax match redifCorrectType /ReDIF-Paper\|ReDIF-Software\|ReDIF-Article\|ReDIF-Chapter\|ReDIF-Book/ contained display

highlight def link redifArgumentType redifError
highlight def link redifCorrectType redifSpecial

"    Dates: Publication-Date, Creation-Date, Revision-Date,
"    Last-Login-Date, Registration-Date
syntax match redifArgumentCreationDate /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectDate contained display
syntax match redifArgumentLastLoginDate /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectDate contained display
syntax match redifArgumentPublicationDate /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectDate contained display
syntax match redifArgumentRegisteredDate /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectDate contained display
syntax match redifArgumentRevisionDate /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectDate contained display
syntax match redifCorrectDate /[1-9]\d\{3}\%(-02\%(-[12]\d\|-0[1-9]\)\=\|-\%(0[469]\|11\)\%(-30\|-[12]\d\|-0[1-9]\)\=\|-\%(0[13578]\|1[02]\)\%(-3[01]\|-[12]\d\|-0[1-9]\)\=\)\=/ contained display

highlight def link redifArgumentCreationDate redifError
highlight def link redifArgumentLastLoginDate redifError
highlight def link redifArgumentPublicationDate redifError
highlight def link redifArgumentRegisteredDate redifError
highlight def link redifArgumentRevisionDate redifError

"    Classification-JEL
syntax match redifArgumentClassificationJEL /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectJEL contained display
syntax match redifCorrectJEL /\<\u\d\{,2}\%([,; \t]\s*\)\=/ contains=redifSpecialJEL contained display
syntax match redifSpecialJEL /\<\u\d\{,2}/ contained display

highlight def link redifArgumentClassificationJEL redifError
highlight def link redifSpecialJEL redifSpecial

"    Pages
syntax match redifArgumentPages /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectPages contained display
syntax match redifCorrectPages /[1-9]\d*-[1-9]\d*/ contained display

highlight def link redifArgumentPages redifError

"    Name-ASCII
syntax match redifArgumentNameASCII /\%(^\S\{-}:\)\@!\S.*/ contains=redifCorrectNameASCII contained display
syntax match redifCorrectNameASCII /[ -~]/ contained display

highlight def link redifArgumentNameASCII redifError

"    Programming-Language
syntax match redifArgumentProgrammingLanguage /\%(^\S\{-}:\)\@!\S.*/ contains=redifGoodProgrammingLanguage contained display
syntax match redifGoodProgrammingLanguage /\<c++\|\<\%(c\|dos executable\|executable\|fortran\|gauss\|gretl\|java\|mathematica\|matlab\|octave\|ox\|perl\|python\|rats\|r\|shazam\|s-plus\|stata\|tsp international\)\>/ contained display

highlight def link redifGoodProgrammingLanguage redifSpecial

"    File-Format
"    TODO The link in the documentation that gives the list of possible formats is broken.
"    ftp://ftp.isi.edu/in-notes/iana/assignments/media-types/media-types
"    These are based on the examples in the documentation.
syntax match redifArgumentFileFormat /\%(^\S\{-}:\)\@!\S.*/ contains=redifGoodFormat contained display
syntax match redifGoodFormat /application\/pdf\|application\/postscript\|text\/html\|text\/plain/ contained display

highlight def link redifGoodFormat redifSpecial

"    Spell-checked arguments
"    Very useful when copy-pasting abstracts that may contain hyphens or
"    ligatures.
syntax region redifArgumentAbstract start=/\%(^\S\{-}:\)\@!\S.*/ end=/^\S\{-}:/me=s-1 contains=@Spell contained
syntax region redifArgumentAvailability start=/\%(^\S\{-}:\)\@!\S.*/ end=/^\S\{-}:/me=s-1 contains=@Spell contained
syntax region redifArgumentBookTitle start=/\%(^\S\{-}:\)\@!\S.*/ end=/^\S\{-}:/me=s-1 contains=@Spell contained
syntax region redifArgumentDescription start=/\%(^\S\{-}:\)\@!\S.*/ end=/^\S\{-}:/me=s-1 contains=@Spell contained
syntax region redifArgumentFileRestriction start=/\%(^\S\{-}:\)\@!\S.*/ end=/^\S\{-}:/me=s-1 contains=@Spell contained
syntax region redifArgumentKeywords start=/\%(^\S\{-}:\)\@!\S.*/ end=/^\S\{-}:/me=s-1 contains=@Spell contained
syntax region redifArgumentNote start=/\%(^\S\{-}:\)\@!\S.*/ end=/^\S\{-}:/me=s-1 contains=@Spell contained
syntax region redifArgumentNotification start=/\%(^\S\{-}:\)\@!\S.*/ end=/^\S\{-}:/me=s-1 contains=@Spell contained
syntax region redifArgumentRestriction start=/\%(^\S\{-}:\)\@!\S.*/ end=/^\S\{-}:/me=s-1 contains=@Spell contained
syntax region redifArgumentTitle start=/\%(^\S\{-}:\)\@!\S.*/ end=/^\S\{-}:/me=s-1 contains=@Spell contained

" Final highlight
highlight def link redifComment Comment
highlight def link redifError Error
highlight def link redifField Identifier
highlight def link redifSpecial Special

" Set "b:current_syntax" to the name of the syntax at the end:
let b:current_syntax="redif"
