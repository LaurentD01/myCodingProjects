package social;

import java.util.List;

import jakarta.persistence.Entity;
import jakarta.persistence.FetchType;
import jakarta.persistence.Id;
import jakarta.persistence.ManyToMany;
import jakarta.persistence.OneToMany;

@Entity
class Person {
  @Id
  private String code;
  private String name;
  private String surname;

  @ManyToMany(fetch = FetchType.EAGER)
  private List<Person> friends;

  @ManyToMany(mappedBy = "partecipanti", fetch = FetchType.EAGER)
  private List<Group> groups;

  @OneToMany(mappedBy = "autore", fetch = FetchType.EAGER)
  private List<Post> posts;

  public Person() {
    // default constructor is needed by JPA
  }

  Person(String code, String name, String surname) {
    this.code = code;
    this.name = name;
    this.surname = surname;
  }

  void addPost(Post p) {
    this.posts.add(p);
  }

  String getCode() {
    return code;
  }

  String getName() {
    return name;
  }

  String getSurname() {
    return surname;
  }

  String getInfo() {
    return code + " " + name + " " + surname;
  }

  List<Person> getFriends(){
    return friends;
  }

  void addGroup(Group g) {
    this.groups.add(g);
  }

  List<Group> getGroups(){
    return groups;
  }


}
