# Social Network


This is an application that supports a social network. All classes are
in the package `social`.

The system uses Hibernate ORM to enable persistence of the information.
To enable proper testing, the class `JPAUtil` is used to retrieve the `EntityManager` 
objects via the `getEntityManager()` method.

---

## Subscription

The interaction with the system is made using class `Social`.

You can register new account using method `addPerson()` which receives
as parameters a unique code, name and surname.

The method throws the exception `PersonExistsException` if the code
passed is already associated to a subscription.

The method `getPerson()` returns a string containing code, name and
surname of the person, in order, separated by blanks. If the code,
passed as a parameter, does not match any person, the method throws the
exception `NoSuchCodeException`.


## Friends

Any person, registered in the social, have possibility to add
friends. Friendship is bidirectional: if person A is friend of a person
B, that means that person B is a friend of a person A.

Friendship is created using method `addFriendship()` that receives as arguments the
codes of both persons. The method throws the exception `NoSuchCodeException`
if one or both codes do not exist.

Method `listOfFriends()` receives as argument the code of a person and
returns the collection of his/her friends. The exception
`NoSuchCodeException` is thrown if the code does not exist. If a
person has no friends, an empty collection is returned.


## Groups

It is possible to register a new group using method `addGroup()`. Name
of the group should be a singleword and must be unique. A `GroupExistsException` is thrown if the group name already exists.

The `updateGroupName()` method allows modifying the name of an existing group. It takes as parameters the current name of the group and the new desired name. A `GroupExistsException` is thrown if the new group name already exists, and a `NoSuchCodeException` is thrown if the current group name does not exist.

The `deleteGroup()` method allows deleting an existing group. It takes the name of the group as a parameter. A `NoSuchCodeException` is thrown if the group name does not exist.

The method `listOfGroups()` returns the list of names of all
registered groups. If there are no groups in the list, the method
returns an empty collection.

A person can subscribe to a group through the method `addPersonToGroup()`
that receives as arguments the code of the person and the name of the
group. In case the code of the person, or the name of the group are
not found a `NoSuchCodeException` is thrown.

Method `listOfPeopleInGroup()` returns collection of codes of people
subscribed to a given group. Returns null if the group does not exist.

## Statistics

Method `personWithLargestNumberOfFriends()` returns code of a person
that has the largest amount of friends (first level). It doesn't consider the
case of ties.

Method `largestGroup()` returns name of the group with the largest number
of members. It doesn't consider the case of ties.

Method `personInLargestNumberOfGroups()` returns code of a person that
is subscribed to the largest number of groups. It doesn't consider the case of
ties.

## Posts

It is possible to add a new post by a given account using the method `post()`
that accepts as arguments the unique code of the person posting, and the text content.
The method returns a unique id for the post containing digits and letters only.

Given a post id it is possible to get:

- the timestamp with `getTimestamp()`,
- the text content with `getPostContent()`.

The timestamp of the post is the current system time at the moment of the post
creation (retrieved through `System.currentTimeMillis()`).

The paginated list of all posts of _a given user_ can be retrieved using the method
`getPaginatedUserPosts()` that accepts the user id, the page number (1 is the first)
and the page length. The method returns the ids of the posts sorted by
descending timestamp. The list is split into pages, each containing a
number of posts specified by the page-length.
E.g. if page length is 5 and page is 2, then posts with position 6 to 10 are returned.
Post are sorted by descenging timestamp, i.e., most recent first.

The paginated list of all posts of _the friends of a given user_ be retrieved using the method
`getPaginatedFriendPosts()` that accepts the user id, the page number (1 is the first)
and the page length. The method works like the previous one but it returns in the list both
the author name and the post id, separated by `":"`.
